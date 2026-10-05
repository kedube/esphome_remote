#pragma once

// Favorite lists as the remote uses them at runtime: from local_entities.h,
// or from the text Home Assistant publishes (see Favorites from Home
// Assistant in the README). Free of ESPHome so tools can test it on a
// computer.

#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "remote_ui_types.h"

struct FavoriteEntity {
    const char *name;
    const char *entity_id;
    // Optional third field. A media player whose device_class is not
    // tv/receiver: its '|'-separated source list (HA does not report a usable
    // source_list for those). A remote: its command set, a name from
    // REMOTE_COMMAND_SETS or seven '|'-separated commands (see the README).
    const char *sources = nullptr;
};

// One mode's favorites: each entity once, in the order the lists first name it.
using EntityEntry = FavoriteEntity;

struct FavoriteList {
    const char *title;
    const FavoriteEntity *entries;
    size_t count;
};

template <size_t Count>
inline constexpr FavoriteList make_favorite_list(const char *title, const FavoriteEntity (&entries)[Count]) {
  return {title, entries, Count};
}

// The persisted UI state keeps the menu position in 5 bits and the position
// in a list in 6.
#ifndef REMOTE_MAX_PERSISTED_FAVORITE_LISTS
#define REMOTE_MAX_PERSISTED_FAVORITE_LISTS 16
#endif
inline constexpr int MAX_PERSISTED_FAVORITE_LISTS = REMOTE_MAX_PERSISTED_FAVORITE_LISTS;
inline constexpr int FAVORITE_LIST_MAX_ITEMS = 64;

// The longest list text the remote takes from Home Assistant, and keeps.
#ifndef REMOTE_FAVORITES_MAX_BYTES
#define REMOTE_FAVORITES_MAX_BYTES 8192
#endif
inline constexpr size_t FAVORITES_MAX_BYTES = REMOTE_FAVORITES_MAX_BYTES;

inline constexpr bool cstr_eq_constexpr(const char *lhs, const char *rhs) {
  if (lhs == rhs) {
    return true;
  }
  if (lhs == nullptr || rhs == nullptr) {
    return false;
  }
  while (*lhs != '\0' && *rhs != '\0') {
    if (*lhs != *rhs) {
      return false;
    }
    ++lhs;
    ++rhs;
  }
  return *lhs == *rhs;
}

inline constexpr bool cstr_starts_with_constexpr(const char *value, const char *prefix) {
  if (value == nullptr || prefix == nullptr) {
    return false;
  }
  while (*prefix != '\0') {
    if (*value == '\0' || *value != *prefix) {
      return false;
    }
    ++value;
    ++prefix;
  }
  return true;
}

inline constexpr RemoteMode favorite_entity_mode_constexpr(const char *entity_id) {
  return cstr_starts_with_constexpr(entity_id, "light.")            ? REMOTE_MODE_LIGHTS :
         (cstr_starts_with_constexpr(entity_id, "switch.") ||
          cstr_starts_with_constexpr(entity_id, "input_boolean."))  ? REMOTE_MODE_SWITCHES :
         cstr_starts_with_constexpr(entity_id, "climate.")          ? REMOTE_MODE_CLIMATE :
         cstr_starts_with_constexpr(entity_id, "water_heater.")     ? REMOTE_MODE_WATER_HEATERS :
         cstr_starts_with_constexpr(entity_id, "humidifier.")       ? REMOTE_MODE_HUMIDIFIERS :
         cstr_starts_with_constexpr(entity_id, "fan.")              ? REMOTE_MODE_FANS :
         (cstr_starts_with_constexpr(entity_id, "cover.") ||
          cstr_starts_with_constexpr(entity_id, "valve."))          ? REMOTE_MODE_COVERS :
         cstr_starts_with_constexpr(entity_id, "lock.")             ? REMOTE_MODE_LOCKS :
         cstr_starts_with_constexpr(entity_id, "media_player.")     ? REMOTE_MODE_MEDIA :
         (cstr_starts_with_constexpr(entity_id, "sensor.") ||
          cstr_starts_with_constexpr(entity_id, "binary_sensor.") ||
          cstr_starts_with_constexpr(entity_id, "person.") ||
          cstr_starts_with_constexpr(entity_id, "device_tracker.") ||
          cstr_starts_with_constexpr(entity_id, "event."))          ? REMOTE_MODE_SENSORS :
         (cstr_starts_with_constexpr(entity_id, "automation.") ||
          cstr_starts_with_constexpr(entity_id, "script.") ||
          cstr_starts_with_constexpr(entity_id, "scene.") ||
          cstr_starts_with_constexpr(entity_id, "button.") ||
          cstr_starts_with_constexpr(entity_id, "input_button."))   ? REMOTE_MODE_AUTOMATION :
         cstr_starts_with_constexpr(entity_id, "alarm_control_panel.") ? REMOTE_MODE_ALARMS :
         cstr_starts_with_constexpr(entity_id, "weather.")          ? REMOTE_MODE_WEATHER :
         (cstr_starts_with_constexpr(entity_id, "number.") ||
          cstr_starts_with_constexpr(entity_id, "input_number.") ||
          cstr_starts_with_constexpr(entity_id, "select.") ||
          cstr_starts_with_constexpr(entity_id, "input_select."))   ? REMOTE_MODE_INPUTS :
         (cstr_starts_with_constexpr(entity_id, "vacuum.") ||
          cstr_starts_with_constexpr(entity_id, "lawn_mower."))     ? REMOTE_MODE_VACUUMS :
         cstr_starts_with_constexpr(entity_id, "timer.")            ? REMOTE_MODE_TIMERS :
         cstr_starts_with_constexpr(entity_id, "remote.")           ? REMOTE_MODE_REMOTES :
                                                                        REMOTE_MODE_INFO;
}

inline RemoteMode favorite_entity_mode(const char *entity_id) {
  return favorite_entity_mode_constexpr(entity_id);
}

// FNV-1a, the hash the wake snapshot keeps of the item on screen.
inline uint32_t favorites_hash_mix(uint32_t hash, const char *data, size_t len) {
  for (size_t i = 0; i < len; i++) {
    hash ^= static_cast<uint8_t>(data[i]);
    hash *= 16777619u;
  }
  return hash;
}

inline constexpr uint32_t FAVORITES_HASH_BASIS = 2166136261u;

inline uint32_t favorites_text_hash(const char *text) {
  return text != nullptr ? favorites_hash_mix(FAVORITES_HASH_BASIS, text, strlen(text)) : FAVORITES_HASH_BASIS;
}

// A Home Assistant entity ID: a domain and an object ID of lower-case
// letters, digits and underscores, joined by one dot.
inline bool favorite_entity_id_well_formed(const char *entity_id) {
  int dots = 0;
  size_t len = 0;
  for (const char *p = entity_id; *p != '\0'; p++, len++) {
    char c = *p;
    if (c == '.') {
      if (++dots > 1 || len == 0 || p[1] == '\0') {
        return false;
      }
    } else if (!((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_')) {
      return false;
    }
  }
  return dots == 1;
}

// The favorite lists in use. Built once at boot and never changed after: the
// trackers and Home Assistant's subscriptions keep pointers into it.
class FavoriteSet {
 public:
  FavoriteSet() = default;
  FavoriteSet(const FavoriteSet &) = delete;
  FavoriteSet &operator=(const FavoriteSet &) = delete;
  FavoriteSet(FavoriteSet &&) = default;
  FavoriteSet &operator=(FavoriteSet &&) = default;

  // Empty lists included (see write_()).
  int list_count() const { return static_cast<int>(this->lists_.size()); }
  const FavoriteList *list(int index) const {
    return index >= 0 && index < this->list_count() ? &this->lists_[index] : nullptr;
  }
  int favorite_count() const {
    int count = 0;
    for (const FavoriteList &list : this->lists_) {
      count += static_cast<int>(list.count);
    }
    return count;
  }

  // One mode's favorites (see EntityEntry).
  int mode_count(RemoteMode mode) const {
    return mode >= 0 && mode < REMOTE_MODE_COUNT ? this->mode_end_[mode] - this->mode_begin_[mode] : 0;
  }
  const EntityEntry *mode_entries(RemoteMode mode) const {
    return this->mode_count(mode) > 0 ? this->mode_list_.data() + this->mode_begin_[mode] : nullptr;
  }

  // Of text(): two sets with the same hash hold the same lists.
  uint32_t hash() const { return this->hash_; }

  // The lists from local_entities.h, in place: their strings live in flash.
  void use(const FavoriteList *lists, size_t count) {
    this->arena_.reset();
    this->entries_.clear();
    this->lists_.assign(lists, lists + count);
    this->index_();
  }

  // The lists in text. Each list starts with a line "#TITLE"; each favorite
  // is a line "Name|entity_id", with the optional third field after another
  // '|'. Blank lines and spaces around fields don't count, and favorites
  // before the first title go in a list called FAVORITES. Takes the text if
  // every line holds; otherwise leaves this set as it was and says what is
  // wrong in *error, and in *error_line the line at fault (0 for none).
  bool parse(const char *text, size_t len, std::string *error, int *error_line = nullptr) {
    std::string canonical;
    int lists = 0;
    int favorites = 0;
    if (!canonicalize_(text, len, &canonical, &lists, &favorites, Failure{error, error_line})) {
      return false;
    }
    this->adopt_(canonical, lists, favorites);
    return true;
  }

  // The lists in the format parse() takes, with nothing it ignores. What the
  // remote saves, and what hash() is of.
  std::string text() const {
    std::string out;
    this->write_([&out](const char *data, size_t len) { out.append(data, len); });
    return out;
  }

 private:
  static bool is_space_(char c) { return c == ' ' || c == '\t' || c == '\r'; }

  static void trim_(const char *&begin, const char *&end) {
    while (begin < end && is_space_(*begin)) {
      begin++;
    }
    while (end > begin && is_space_(end[-1])) {
      end--;
    }
  }

  struct Failure {
    std::string *error;
    int *line;
  };

  static bool fail_(const Failure &failure, int line, const std::string &message) {
    if (failure.error != nullptr) {
      *failure.error = line > 0 ? "Line " + std::to_string(line) + ": " + message : message;
    }
    if (failure.line != nullptr) {
      *failure.line = line;
    }
    return false;
  }

  // Checks text and writes it out without what parse() ignores.
  static bool canonicalize_(const char *text, size_t len, std::string *out, int *list_count, int *favorite_count,
                            const Failure &error) {
    if (text == nullptr) {
      len = 0;
    }
    if (len > FAVORITES_MAX_BYTES) {
      return fail_(error, 0,
                   "the list is " + std::to_string(len) + " bytes; the remote takes up to " +
                       std::to_string(FAVORITES_MAX_BYTES));
    }
    out->clear();
    out->reserve(len);
    std::string title = "FAVORITES";
    int title_line = 0;
    bool title_used = false;
    int lists = 0;
    int in_list = 0;
    int favorites = 0;
    int line = 0;
    const char *end_of_text = text + len;
    for (const char *p = text; p < end_of_text;) {
      const char *newline = static_cast<const char *>(memchr(p, '\n', end_of_text - p));
      const char *begin = p;
      const char *end = newline != nullptr ? newline : end_of_text;
      p = newline != nullptr ? newline + 1 : end_of_text;
      line++;
      trim_(begin, end);
      if (begin == end) {
        continue;
      }
      if (memchr(begin, '\0', end - begin) != nullptr) {
        return fail_(error, line, "holds a NUL character");
      }
      if (*begin == '#') {
        const char *title_begin = begin + 1;
        trim_(title_begin, end);
        if (title_begin == end) {
          return fail_(error, line, "a list needs a title after #");
        }
        title.assign(title_begin, end);
        title_line = line;
        title_used = false;
        continue;
      }

      const char *bar = static_cast<const char *>(memchr(begin, '|', end - begin));
      if (bar == nullptr) {
        return fail_(error, line, "expected Name|entity_id");
      }
      const char *name_begin = begin;
      const char *name_end = bar;
      trim_(name_begin, name_end);
      const char *entity_begin = bar + 1;
      const char *second_bar = static_cast<const char *>(memchr(entity_begin, '|', end - entity_begin));
      const char *entity_end = second_bar != nullptr ? second_bar : end;
      trim_(entity_begin, entity_end);
      const char *extra_begin = second_bar != nullptr ? second_bar + 1 : end;
      const char *extra_end = end;
      trim_(extra_begin, extra_end);

      std::string name(name_begin, name_end);
      std::string entity_id(entity_begin, entity_end);
      std::string extra(extra_begin, extra_end);
      if (name.empty()) {
        return fail_(error, line, "the favorite needs a name before the |");
      }
      if (entity_id.empty()) {
        return fail_(error, line, "the favorite needs an entity ID after the |");
      }
      if (!favorite_entity_id_well_formed(entity_id.c_str())) {
        return fail_(error, line,
                     entity_id + " isn't an entity ID (lower-case letters, digits and _, with one dot)");
      }
      RemoteMode mode = favorite_entity_mode(entity_id.c_str());
      if (mode == REMOTE_MODE_INFO) {
        return fail_(error, line, entity_id + ": the remote doesn't support this domain");
      }
      if (mode == REMOTE_MODE_REMOTES && !remote_command_field_valid(extra.c_str())) {
        return fail_(error, line,
                     "a remote's third field must be apple_tv, android_tv, roku, samsung, bravia, philips or seven "
                     "'|'-separated commands");
      }

      if (!title_used) {
        if (lists == MAX_PERSISTED_FAVORITE_LISTS) {
          return fail_(error, title_line > 0 ? title_line : line,
                       "more than " + std::to_string(MAX_PERSISTED_FAVORITE_LISTS) + " lists");
        }
        out->push_back('#');
        out->append(title);
        out->push_back('\n');
        lists++;
        in_list = 0;
        title_used = true;
      }
      if (++in_list > FAVORITE_LIST_MAX_ITEMS) {
        return fail_(error, line,
                     "more than " + std::to_string(FAVORITE_LIST_MAX_ITEMS) + " favorites in " + title);
      }
      out->append(name);
      out->push_back('|');
      out->append(entity_id);
      if (!extra.empty()) {
        out->push_back('|');
        out->append(extra);
      }
      out->push_back('\n');
      favorites++;
    }
    if (favorites == 0) {
      return fail_(error, 0, "the list has no favorites");
    }
    *list_count = lists;
    *favorite_count = favorites;
    return true;
  }

  // Takes text from canonicalize_(), splitting it in place.
  void adopt_(const std::string &canonical, int list_count, int favorite_count) {
    std::unique_ptr<char[]> arena(new char[canonical.size() + 1]);
    memcpy(arena.get(), canonical.data(), canonical.size());
    arena[canonical.size()] = '\0';

    std::vector<FavoriteEntity> entries;
    entries.reserve(favorite_count);
    std::vector<FavoriteList> lists;
    lists.reserve(list_count);
    std::vector<size_t> starts;
    starts.reserve(list_count);
    for (char *p = arena.get(); *p != '\0';) {
      char *line = p;
      char *newline = strchr(p, '\n');
      *newline = '\0';
      p = newline + 1;
      if (line[0] == '#') {
        lists.push_back({line + 1, nullptr, 0});
        starts.push_back(entries.size());
        continue;
      }
      char *entity_id = strchr(line, '|');
      *entity_id++ = '\0';
      char *extra = strchr(entity_id, '|');
      if (extra != nullptr) {
        *extra++ = '\0';
      }
      entries.push_back({line, entity_id, extra});
      lists.back().count++;
    }
    for (size_t i = 0; i < lists.size(); i++) {
      lists[i].entries = entries.data() + starts[i];
    }

    this->arena_ = std::move(arena);
    this->entries_ = std::move(entries);
    this->lists_ = std::move(lists);
    this->index_();
  }

  // An empty list is left out, as parse() leaves it out: local_entities.h
  // may have one ({"OUTDOOR", nullptr, 0}), which the menu skips.
  template <typename Emit>
  void write_(Emit emit) const {
    auto emit_text = [&emit](const char *text) { emit(text != nullptr ? text : "", text != nullptr ? strlen(text) : 0); };
    for (const FavoriteList &list : this->lists_) {
      if (list.count == 0) {
        continue;
      }
      emit("#", 1);
      emit_text(list.title);
      emit("\n", 1);
      for (size_t i = 0; i < list.count; i++) {
        const FavoriteEntity &entry = list.entries[i];
        emit_text(entry.name);
        emit("|", 1);
        emit_text(entry.entity_id);
        if (entry.sources != nullptr && entry.sources[0] != '\0') {
          emit("|", 1);
          emit(entry.sources, strlen(entry.sources));
        }
        emit("\n", 1);
      }
    }
  }

  // Groups the favorites by mode, each entity once, and hashes the lists.
  void index_() {
    int total = this->favorite_count();
    this->mode_list_.clear();
    this->mode_list_.reserve(total);
    std::vector<uint32_t> hashes;
    hashes.reserve(total);
    for (int mode = 0; mode < REMOTE_MODE_COUNT; mode++) {
      int begin = static_cast<int>(this->mode_list_.size());
      this->mode_begin_[mode] = begin;
      for (const FavoriteList &list : this->lists_) {
        for (size_t i = 0; i < list.count; i++) {
          const FavoriteEntity &entry = list.entries[i];
          if (favorite_entity_mode(entry.entity_id) != mode) {
            continue;
          }
          uint32_t hash = favorites_text_hash(entry.entity_id);
          bool seen = false;
          for (int j = begin; j < static_cast<int>(this->mode_list_.size()) && !seen; j++) {
            seen = hashes[j] == hash && strcmp(this->mode_list_[j].entity_id, entry.entity_id) == 0;
          }
          if (!seen) {
            this->mode_list_.push_back(entry);
            hashes.push_back(hash);
          }
        }
      }
      this->mode_end_[mode] = static_cast<int>(this->mode_list_.size());
    }

    uint32_t hash = FAVORITES_HASH_BASIS;
    this->write_([&hash](const char *data, size_t len) { hash = favorites_hash_mix(hash, data, len); });
    this->hash_ = hash;
  }

  std::unique_ptr<char[]> arena_;
  std::vector<FavoriteEntity> entries_;
  std::vector<FavoriteList> lists_;
  std::vector<EntityEntry> mode_list_;
  int mode_begin_[REMOTE_MODE_COUNT] = {};
  int mode_end_[REMOTE_MODE_COUNT] = {};
  uint32_t hash_ = FAVORITES_HASH_BASIS;
};
