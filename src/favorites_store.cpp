#include <cstddef>
#include <utility>

// local_entities.h before favorites_store.h, which only fills in what it
// leaves out.
#include "favorites_lists.h"
#include "local_entities.h"
#include "favorites_store.h"

#ifdef USE_ESP32
#include <esp_attr.h>
#include <esp_err.h>
#include <esp_system.h>
#include <nvs.h>

#include "esphome/core/log.h"
#else
// Built on a computer by tools that test this file: the list is kept in
// memory, and log lines go to stderr.
#include <cstdio>
#define RTC_NOINIT_ATTR
#define ESP_LOGI(tag, ...) (fprintf(stderr, "[I][%s] ", tag), fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n"))
#define ESP_LOGW(tag, ...) (fprintf(stderr, "[W][%s] ", tag), fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n"))
#define ESP_LOGE(tag, ...) (fprintf(stderr, "[E][%s] ", tag), fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n"))
std::string favorites_test_storage;
bool favorites_test_storage_set = false;
bool favorites_test_storage_fails = false;
bool favorites_test_crashed = true;
#endif

namespace {

const char *const TAG = "favorites";

inline constexpr size_t BUILTIN_LIST_COUNT = sizeof(FAVORITE_LISTS) / sizeof(FAVORITE_LISTS[0]);

// Boots in a row with the saved list that crashed before Home Assistant sent
// its list or the remote went to sleep. After this many, the remote stops
// using it.
constexpr uint32_t MAX_UNFINISHED_BOOTS = 3;

// Home Assistant's limit on a state.
constexpr size_t STATUS_MAX_BYTES = 255;

// Kept in RTC memory, which survives deep sleep and restarts, crashes
// included, but not a power cut.
struct BootGuard {
  uint32_t magic;
  uint32_t unfinished_boots;  // see MAX_UNFINISHED_BOOTS
  uint32_t bad_hash;          // a saved list the remote kept crashing with
  uint32_t restarted_for;     // the list the remote last restarted to use
  uint32_t save_failed;       // a list the remote couldn't save at sleep
  uint32_t check;
};
constexpr uint32_t GUARD_MAGIC = 0x46415631;  // "FAV1"

RTC_NOINIT_ATTR BootGuard rtc_guard;

uint32_t guard_check(const BootGuard &guard) {
  return favorites_hash_mix(FAVORITES_HASH_BASIS, reinterpret_cast<const char *>(&guard),
                            offsetof(BootGuard, check));
}

BootGuard &guard() {
  if (rtc_guard.magic != GUARD_MAGIC || rtc_guard.check != guard_check(rtc_guard)) {
    rtc_guard = BootGuard{};
    rtc_guard.magic = GUARD_MAGIC;
    rtc_guard.check = guard_check(rtc_guard);
  }
  return rtc_guard;
}

void store_guard() { rtc_guard.check = guard_check(rtc_guard); }

#ifdef USE_ESP32
// A crash, rather than a flat battery's brownout, a power cut or a wake.
bool last_reset_was_crash() {
  switch (esp_reset_reason()) {
    case ESP_RST_PANIC:
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:
      return true;
    default:
      return false;
  }
}

const char *const NVS_NAMESPACE = "remote_favs";
const char *const NVS_KEY = "lists";

bool storage_load(std::string *text) {
  nvs_handle_t handle;
  if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
    return false;  // never saved
  }
  size_t len = 0;
  esp_err_t err = nvs_get_blob(handle, NVS_KEY, nullptr, &len);
  if (err == ESP_OK && (len == 0 || len > FAVORITES_MAX_BYTES)) {
    err = ESP_ERR_INVALID_SIZE;
  }
  if (err == ESP_OK) {
    text->resize(len);
    err = nvs_get_blob(handle, NVS_KEY, &(*text)[0], &len);
  }
  nvs_close(handle);
  return err == ESP_OK;
}

// Written straight to NVS, not through ESPHome's preferences, so the result
// is known: the remote must not restart for a list it didn't keep.
bool storage_save(const std::string &text, std::string *error) {
  nvs_handle_t handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
  if (err == ESP_OK) {
    err = nvs_set_blob(handle, NVS_KEY, text.data(), text.size());
    if (err == ESP_OK) {
      err = nvs_commit(handle);
    }
    nvs_close(handle);
  }
  std::string check;
  if (err == ESP_OK && (!storage_load(&check) || check != text)) {
    err = ESP_FAIL;
  }
  if (err != ESP_OK) {
    *error = esp_err_to_name(err);
  }
  return err == ESP_OK;
}
#else
bool last_reset_was_crash() { return favorites_test_crashed; }

bool storage_load(std::string *text) {
  if (!favorites_test_storage_set) {
    return false;
  }
  *text = favorites_test_storage;
  return true;
}

bool storage_save(const std::string &text, std::string *error) {
  if (favorites_test_storage_fails) {
    *error = "ESP_ERR_NVS_NOT_ENOUGH_SPACE";
    return false;
  }
  favorites_test_storage = text;
  favorites_test_storage_set = true;
  return true;
}
#endif

struct State {
  bool loaded = false;
  FavoriteSet set;
  bool from_home_assistant = false;
  uint32_t stored_hash = 0;  // of the saved list; 0 for none
  std::string pending;       // a list to save at sleep
  uint32_t pending_hash = 0;
  uint32_t last_payload_hash = 0;
  bool restart = false;
  std::string status;
  FavoritesNote note = FAVORITES_NOTE_NONE;
  int note_line = 0;
  void (*listener)(const std::string &) = nullptr;
};

State &state() {
  static State state;
  return state;
}

int lists_with_favorites(const FavoriteSet &set) {
  int lists = 0;
  for (int i = 0; i < set.list_count(); i++) {
    lists += set.list(i)->count > 0 ? 1 : 0;
  }
  return lists;
}

std::string describe(const FavoriteSet &set) {
  int lists = lists_with_favorites(set);
  int favorites = set.favorite_count();
  return std::to_string(lists) + (lists == 1 ? " list, " : " lists, ") + std::to_string(favorites) +
         (favorites == 1 ? " favorite" : " favorites");
}

std::string still_using(const State &st) {
  return st.from_home_assistant ? "Still using the previous list." : "Still using local_entities.h.";
}

void set_status(State &st, std::string status, FavoritesNote note = FAVORITES_NOTE_NONE, int note_line = 0) {
  st.note = note;
  st.note_line = note_line;
  if (status.size() > STATUS_MAX_BYTES) {
    size_t cut = STATUS_MAX_BYTES;
    while (cut > 0 && (static_cast<uint8_t>(status[cut]) & 0xC0) == 0x80) {
      cut--;  // not inside a UTF-8 character
    }
    status.resize(cut);
  }
  if (status == st.status) {
    return;
  }
  ESP_LOGI(TAG, "%s", status.c_str());
  st.status = std::move(status);
  if (st.listener != nullptr) {
    st.listener(st.status);
  }
}

void reject(State &st, std::string why, int line = 0, FavoritesNote note = FAVORITES_NOTE_NOT_USED) {
  if (!why.empty() && why[0] >= 'A' && why[0] <= 'Z') {
    why[0] = static_cast<char>(why[0] - 'A' + 'a');
  }
  ESP_LOGW(TAG, "List from Home Assistant not used: %s", why.c_str());
  set_status(st, "Not used: " + why + ". " + still_using(st), note, line);
}

// The remote booted, synced and is working: the saved list doesn't crash it.
void boot_finished() {
  BootGuard &g = guard();
  if (g.unfinished_boots != 0) {
    g.unfinished_boots = 0;
    store_guard();
  }
}

void load(State &st) {
  st.loaded = true;
  st.set.use(FAVORITE_LISTS, BUILTIN_LIST_COUNT);
  std::string local = "local_entities.h: " + describe(st.set);
  std::string text;
  if (!favorites_from_home_assistant_enabled() || !storage_load(&text)) {
    set_status(st, local);
    return;
  }
  FavoriteSet saved;
  std::string error;
  if (!saved.parse(text.data(), text.size(), &error)) {
    ESP_LOGW(TAG, "The saved list can't be read: %s", error.c_str());
    set_status(st, local + ". The saved list from Home Assistant can't be read.", FAVORITES_NOTE_NOT_USED);
    return;
  }
  st.stored_hash = saved.hash();
  BootGuard &g = guard();
  if (!last_reset_was_crash()) {
    g.unfinished_boots = 0;
  }
  if (g.bad_hash != st.stored_hash && g.unfinished_boots >= MAX_UNFINISHED_BOOTS) {
    ESP_LOGE(TAG, "Crashed %u times in a row with the saved list; using local_entities.h",
             static_cast<unsigned>(g.unfinished_boots));
    g.bad_hash = st.stored_hash;
  }
  if (g.bad_hash == st.stored_hash) {
    g.unfinished_boots = 0;
    store_guard();
    set_status(st, local + ". The remote kept crashing with the list from Home Assistant, so it isn't used.",
               FAVORITES_NOTE_SET_ASIDE);
    return;
  }
  g.unfinished_boots++;
  store_guard();
  st.set = std::move(saved);
  st.from_home_assistant = true;
  set_status(st, "Home Assistant: " + describe(st.set));
}

State &loaded_state() {
  State &st = state();
  if (!st.loaded) {
    load(st);
  }
  return st;
}

}  // namespace

const FavoriteSet &active_favorites() { return loaded_state().set; }

bool active_favorites_from_home_assistant() { return loaded_state().from_home_assistant; }

void favorites_received(const char *text, size_t len, FavoritesFitCheck fits, bool may_restart) {
  State &st = loaded_state();
  boot_finished();
  if (!favorites_from_home_assistant_enabled() || text == nullptr || ha_state_missing(text, len)) {
    return;
  }
  // Home Assistant sends every subscribed value again when it reconnects.
  uint32_t payload_hash = favorites_hash_mix(FAVORITES_HASH_BASIS, text, len);
  if (payload_hash == st.last_payload_hash) {
    return;
  }
  st.last_payload_hash = payload_hash;

  FavoriteSet received;
  std::string error;
  int error_line = 0;
  if (!received.parse(text, len, &error, &error_line)) {
    reject(st, error, error_line);
    return;
  }
  uint32_t hash = received.hash();
  BootGuard &g = guard();
  if (hash == g.bad_hash) {
    reject(st, "the remote kept crashing with this list", 0, FAVORITES_NOTE_SET_ASIDE);
    return;
  }
  // Only lists the remote doesn't hold already: it measures free memory with
  // this message and the parsed copy still taking some.
  if (fits != nullptr && hash != st.set.hash() && hash != st.stored_hash) {
    std::string why = fits(received, st.set);
    if (!why.empty()) {
      reject(st, why);
      return;
    }
  }

  // The saved list follows Home Assistant, changes undone included.
  uint32_t saved_next = st.pending.empty() ? st.stored_hash : st.pending_hash;
  if (hash != saved_next) {
    if (hash == st.stored_hash) {
      st.pending.clear();
      st.pending_hash = 0;
    } else {
      st.pending = received.text();
      st.pending_hash = hash;
    }
  }

  if (hash == st.set.hash()) {
    set_status(st, "Home Assistant: " + describe(st.set));
    return;
  }
  // Restart to use this list now if the remote is still on local_entities.h,
  // or isn't in use. Once per list, so a list that fails to load can't
  // restart the remote over and over.
  if ((!st.from_home_assistant || may_restart) && g.restarted_for != hash) {
    std::string save_error;
    if (!storage_save(received.text(), &save_error)) {
      ESP_LOGE(TAG, "Couldn't save the list: %s", save_error.c_str());
      set_status(st, "Couldn't save the list from Home Assistant (" + save_error + "). " + still_using(st),
                 FAVORITES_NOTE_NOT_SAVED);
      return;
    }
    st.stored_hash = hash;
    st.pending.clear();
    st.pending_hash = 0;
    g.restarted_for = hash;
    g.save_failed = 0;
    store_guard();
    st.restart = true;
    set_status(st, "Restarting to use " + describe(received) + " from Home Assistant", FAVORITES_NOTE_RESTARTING);
    return;
  }
  if (g.save_failed == hash) {
    set_status(st,
               "The remote couldn't save " + describe(received) +
                   " from Home Assistant last time; it tries again as it goes to sleep. " + still_using(st),
               FAVORITES_NOTE_NOT_SAVED);
    return;
  }
  set_status(st, "Next wake: " + describe(received) + " from Home Assistant", FAVORITES_NOTE_NEXT_WAKE);
}

void favorites_save_pending() {
  State &st = loaded_state();
  boot_finished();
  if (st.pending.empty()) {
    return;
  }
  BootGuard &g = guard();
  std::string error;
  if (storage_save(st.pending, &error)) {
    ESP_LOGI(TAG, "Saved the list from Home Assistant");
    st.stored_hash = st.pending_hash;
    g.save_failed = 0;
  } else {
    ESP_LOGE(TAG, "Couldn't save the list from Home Assistant: %s", error.c_str());
    g.save_failed = st.pending_hash;
  }
  store_guard();
  st.pending.clear();
  st.pending_hash = 0;
}

bool favorites_restart_requested() { return state().restart; }

FavoritesSummary favorites_summary() {
  const State &st = loaded_state();
  FavoritesSummary summary;
  summary.from_home_assistant = st.from_home_assistant;
  summary.lists = lists_with_favorites(st.set);
  summary.favorites = st.set.favorite_count();
  summary.note = st.note;
  summary.note_line = st.note_line;
  return summary;
}

void favorites_set_status_listener(void (*listener)(const std::string &status)) {
  State &st = loaded_state();
  st.listener = listener;
  if (listener != nullptr) {
    listener(st.status);
  }
}

#ifndef USE_ESP32
// A restart, for the tests: RTC memory and the saved list outlive it.
void favorites_test_restart() { state() = State{}; }
#endif
