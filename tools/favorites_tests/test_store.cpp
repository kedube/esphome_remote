// Tests which favorite lists the remote uses, and what it does with the ones
// Home Assistant sends (src/favorites_store.cpp, built for the computer: it
// keeps the saved list in memory). Uses the lists in local_entities.h next to
// this file. Run by run.py.

#include <cstdio>
#include <cstring>
#include <string>

#include "favorites_lists.h"
#include "local_entities.h"
#include "favorites_store.h"

extern std::string favorites_test_storage;
extern bool favorites_test_storage_set;
extern bool favorites_test_storage_fails;
extern bool favorites_test_crashed;
void favorites_test_restart();

static int failures = 0;
#define CHECK(cond)                                                   \
  do {                                                                \
    if (!(cond)) {                                                    \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
      failures++;                                                     \
    }                                                                 \
  } while (0)

static std::string status;
static void listen(const std::string &text) { status = text; }

// in_use: a button was pressed since the remote woke.
static void send(const char *text, bool in_use = true, FavoritesFitCheck fits = nullptr) {
  favorites_received(text, strlen(text), fits, !in_use);
}
static void restart() {
  favorites_test_restart();
  favorites_set_status_listener(listen);
}
static bool starts(const std::string &text, const char *prefix) { return text.rfind(prefix, 0) == 0; }

static const char *const X = "#ONE\nLamp|light.lamp\nDesk|light.desk\n";
static const char *const Y = "#ONE\nLamp|light.lamp\nDesk|light.desk\n#TWO\nTV|remote.tv|roku\n";
static const char *const Z = "#THREE\nPorch|light.porch\n";
static const char *const W = "#W\nA|light.a\nB|light.b\nC|light.c\n";

int main() {
  // local_entities.h until Home Assistant sends lists.
  restart();
  CHECK(!active_favorites_from_home_assistant());
  CHECK(status == "local_entities.h: 2 lists, 6 favorites");
  FavoritesSummary summary = favorites_summary();
  CHECK(!summary.from_home_assistant && summary.lists == 2 && summary.favorites == 6);
  CHECK(summary.note == FAVORITES_NOTE_NONE);
  CHECK(active_favorites().mode_count(REMOTE_MODE_LIGHTS) == 3);

  // The first lists restart the remote, in use or not, saved first.
  send(X, true);
  CHECK(favorites_restart_requested());
  CHECK(status == "Restarting to use 1 list, 2 favorites from Home Assistant");
  CHECK(favorites_summary().note == FAVORITES_NOTE_RESTARTING);
  CHECK(favorites_test_storage == X);

  restart();
  CHECK(active_favorites_from_home_assistant());
  CHECK(status == "Home Assistant: 1 list, 2 favorites");
  summary = favorites_summary();
  CHECK(summary.from_home_assistant && summary.lists == 1 && summary.favorites == 2);
  CHECK(!favorites_restart_requested());
  send(X);  // the same lists again: nothing to do
  CHECK(status == "Home Assistant: 1 list, 2 favorites" && !favorites_restart_requested());

  // An edit while the remote is in use: saved at sleep, used from the next wake.
  send(Y, true);
  CHECK(status == "Next wake: 2 lists, 3 favorites from Home Assistant");
  CHECK(favorites_summary().note == FAVORITES_NOTE_NEXT_WAKE);
  CHECK(favorites_test_storage == X && !favorites_restart_requested());
  send(X);  // undone before sleep: nothing to save
  CHECK(status == "Home Assistant: 1 list, 2 favorites");
  favorites_save_pending();
  CHECK(favorites_test_storage == X);

  // An edit while nobody has touched the remote since it woke: restart now.
  send(Y, false);
  CHECK(favorites_restart_requested());
  CHECK(status == "Restarting to use 2 lists, 3 favorites from Home Assistant");
  CHECK(favorites_test_storage == Y);
  restart();
  CHECK(status == "Home Assistant: 2 lists, 3 favorites");
  CHECK(active_favorites().mode_count(REMOTE_MODE_REMOTES) == 1);

  // Bad lists are turned down, with the line at fault, and the old ones stay.
  send("#ONE\nLamp|Light.Lamp\n");
  CHECK(starts(status, "Not used: line 2: Light.Lamp isn't an entity ID"));
  CHECK(status.find("Still using the previous list.") != std::string::npos);
  summary = favorites_summary();
  CHECK(summary.note == FAVORITES_NOTE_NOT_USED && summary.note_line == 2 && summary.favorites == 3);
  favorites_save_pending();
  CHECK(favorites_test_storage == Y);

  // Resets that aren't crashes (a flat battery's brownouts) never set the lists aside.
  favorites_test_crashed = false;
  for (int i = 0; i < 6; i++) {
    restart();
    CHECK(active_favorites_from_home_assistant());
  }
  favorites_test_crashed = true;

  // Three crashes running as it starts, then local_entities.h.
  favorites_save_pending();  // this boot finishes
  restart();
  restart();
  restart();
  CHECK(active_favorites_from_home_assistant());
  restart();
  CHECK(!active_favorites_from_home_assistant());
  CHECK(status ==
        "local_entities.h: 2 lists, 6 favorites. The remote kept crashing with the list from Home Assistant, so it "
        "isn't used.");
  CHECK(favorites_summary().note == FAVORITES_NOTE_SET_ASIDE);
  restart();  // and they stay set aside
  CHECK(!active_favorites_from_home_assistant());
  send(Y, false);
  CHECK(status == "Not used: the remote kept crashing with this list. Still using local_entities.h.");
  CHECK(favorites_summary().note == FAVORITES_NOTE_SET_ASIDE && !favorites_restart_requested());
  send(Z);  // fixed lists: restart to use them
  CHECK(favorites_restart_requested() && favorites_test_storage == Z);
  restart();
  CHECK(active_favorites_from_home_assistant() && status == "Home Assistant: 1 list, 1 favorite");

  // A save that fails at sleep is reported on the next wake.
  send(X);
  favorites_test_storage_fails = true;
  favorites_save_pending();
  favorites_test_storage_fails = false;
  CHECK(favorites_test_storage == Z);
  restart();
  send(X);
  CHECK(starts(status, "The remote couldn't save 1 list, 2 favorites from Home Assistant last time"));
  CHECK(favorites_summary().note == FAVORITES_NOTE_NOT_SAVED);
  favorites_save_pending();
  CHECK(favorites_test_storage == X);
  restart();
  send(X);
  CHECK(status == "Home Assistant: 1 list, 2 favorites");

  // Lists already held skip the memory check: it measures with the incoming
  // message still taking memory.
  static int checks = 0;
  auto too_big = [](const FavoriteSet &, const FavoriteSet &) -> std::string {
    checks++;
    return "too big for the remote's memory: it needs about 61 KB, and about 38 KB is free for favorites";
  };
  restart();
  send(X, true, too_big);
  CHECK(checks == 0 && status == "Home Assistant: 1 list, 2 favorites");
  send(W, true, too_big);
  CHECK(checks == 1);
  CHECK(status ==
        "Not used: too big for the remote's memory: it needs about 61 KB, and about 38 KB is free for favorites. "
        "Still using the previous list.");
  summary = favorites_summary();
  CHECK(summary.note == FAVORITES_NOTE_NOT_USED && summary.note_line == 0);

  // Empty, missing and unavailable values are ignored.
  std::string before = status;
  for (const char *missing : {"", "unknown", "unavailable", "None"}) {
    send(missing);
  }
  CHECK(status == before);

  // One restart per lists, so lists that won't load can't restart the remote
  // over and over.
  favorites_test_storage_set = false;
  restart();
  CHECK(!active_favorites_from_home_assistant());
  send(Z, false);  // the remote restarted for Z before
  CHECK(!favorites_restart_requested());
  CHECK(status == "Next wake: 1 list, 1 favorite from Home Assistant");
  send(W);  // but not for W
  CHECK(favorites_restart_requested());
  favorites_test_storage = "garbage";
  favorites_test_storage_set = true;
  restart();
  CHECK(status == "local_entities.h: 2 lists, 6 favorites. The saved list from Home Assistant can't be read.");
  CHECK(favorites_summary().note == FAVORITES_NOTE_NOT_USED);
  send(W, false);  // restarted for W last
  CHECK(!favorites_restart_requested());

  // Statuses are cut to Home Assistant's 255 bytes, never inside a character.
  std::string long_line = "#A\nL|" + std::string(300, 'x') + "\xC3\xA9|\n";
  send(long_line.c_str());
  CHECK(status.size() <= 255 && (static_cast<uint8_t>(status.back()) & 0xC0) != 0xC0);

  printf(failures == 0 ? "favorites store: all passed\n" : "favorites store: %d failed\n", failures);
  return failures == 0 ? 0 : 1;
}
