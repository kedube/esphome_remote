// Tests the favorite-list parser (include/favorites_lists.h). Run by run.py.

#include <cstdio>
#include <cstdlib>
#include <string>

#include "favorites_lists.h"

static int failures = 0;
#define CHECK(cond)                                                   \
  do {                                                                \
    if (!(cond)) {                                                    \
      printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);          \
      failures++;                                                     \
    }                                                                 \
  } while (0)

static std::string error_of(const std::string &text, int *line = nullptr) {
  FavoriteSet set;
  std::string error;
  return set.parse(text.data(), text.size(), &error, line) ? "" : error;
}

static void test_parse() {
  const std::string text =
      "  Lamp | light.lamp \r\n"
      "\n"
      "#OFFICE\r\n"
      "Light|light.office_light\n"
      "Recessed|light.office_recessed\n"
      "TV|remote.den_tv|samsung\n"
      "Speaker|media_player.spk|Spotify|Radio | Line In\n"
      "#EMPTY\n"
      "#KITCHEN\n"
      "Light again|light.office_light\n"
      "Temp|sensor.kitchen_temp\n";
  FavoriteSet set;
  std::string error;
  CHECK(set.parse(text.data(), text.size(), &error));
  CHECK(set.list_count() == 3);
  CHECK(std::string(set.list(0)->title) == "FAVORITES");  // before the first title
  CHECK(set.list(0)->count == 1 && std::string(set.list(0)->entries[0].name) == "Lamp");
  CHECK(std::string(set.list(0)->entries[0].entity_id) == "light.lamp");
  CHECK(set.list(0)->entries[0].sources == nullptr);
  CHECK(std::string(set.list(1)->title) == "OFFICE" && set.list(1)->count == 4);
  CHECK(std::string(set.list(1)->entries[2].sources) == "samsung");
  CHECK(std::string(set.list(1)->entries[3].sources) == "Spotify|Radio | Line In");
  CHECK(std::string(set.list(2)->title) == "KITCHEN" && set.list(2)->count == 2);  // EMPTY dropped
  CHECK(set.list(3) == nullptr && set.list(-1) == nullptr);
  CHECK(set.favorite_count() == 7);
  // Each entity once per mode, named as the lists first name it.
  CHECK(set.mode_count(REMOTE_MODE_LIGHTS) == 3);
  CHECK(std::string(set.mode_entries(REMOTE_MODE_LIGHTS)[1].name) == "Light");
  CHECK(set.mode_count(REMOTE_MODE_REMOTES) == 1);
  CHECK(set.mode_count(REMOTE_MODE_MEDIA) == 1);
  CHECK(set.mode_count(REMOTE_MODE_SENSORS) == 1);
  CHECK(set.mode_count(REMOTE_MODE_CLIMATE) == 0 && set.mode_entries(REMOTE_MODE_CLIMATE) == nullptr);
  CHECK(set.mode_count(REMOTE_MODE_INFO) == 0);

  const std::string canonical =
      "#FAVORITES\nLamp|light.lamp\n#OFFICE\nLight|light.office_light\nRecessed|light.office_recessed\n"
      "TV|remote.den_tv|samsung\nSpeaker|media_player.spk|Spotify|Radio | Line In\n#KITCHEN\n"
      "Light again|light.office_light\nTemp|sensor.kitchen_temp\n";
  CHECK(set.text() == canonical);
  FavoriteSet again;
  CHECK(again.parse(canonical.data(), canonical.size(), &error));
  CHECK(again.hash() == set.hash() && again.text() == canonical);

  // Moving keeps every pointer good.
  FavoriteSet moved = std::move(again);
  CHECK(moved.text() == canonical && moved.hash() == set.hash());
  CHECK(std::string(moved.mode_entries(REMOTE_MODE_LIGHTS)[2].entity_id) == "light.office_recessed");

  // No trailing newline; a title with spaces around it.
  FavoriteSet trimmed;
  const std::string bare = "# Living Room \nLamp|light.lamp";
  CHECK(trimmed.parse(bare.data(), bare.size(), &error));
  CHECK(std::string(trimmed.list(0)->title) == "Living Room");
  CHECK(trimmed.text() == "#Living Room\nLamp|light.lamp\n");
}

static void test_builtin() {
  static const FavoriteEntity A[] = {{"Lamp", "light.lamp"}};
  static const FavoriteEntity B[] = {{"TV", "remote.den_tv", "samsung"}, {"Spk", "media_player.spk", "A|B"}};
  static const FavoriteList LISTS[] = {make_favorite_list("ONE", A), {"OUTDOOR", nullptr, 0},
                                       make_favorite_list("TWO", B)};
  const std::string same = "#ONE\nLamp|light.lamp\n#TWO\nTV|remote.den_tv|samsung\nSpk|media_player.spk|A|B\n";
  FavoriteSet builtin;
  builtin.use(LISTS, 3);
  FavoriteSet parsed;
  std::string error;
  CHECK(parsed.parse(same.data(), same.size(), &error));
  // The empty list keeps its menu slot but isn't in the text.
  CHECK(builtin.text() == same && builtin.hash() == parsed.hash());
  CHECK(builtin.list_count() == 3 && builtin.list(1)->count == 0 && builtin.favorite_count() == 3);
  CHECK(builtin.mode_count(REMOTE_MODE_REMOTES) == 1);

  // A missing name or title doesn't crash.
  static const FavoriteEntity NAMELESS[] = {{nullptr, "light.lamp"}};
  static const FavoriteList UNTITLED[] = {{nullptr, NAMELESS, 1}};
  FavoriteSet odd;
  odd.use(UNTITLED, 1);
  CHECK(odd.text() == "#\n|light.lamp\n");

  // A failed parse leaves the set as it was.
  uint32_t before = builtin.hash();
  CHECK(!builtin.parse("nonsense", 8, &error));
  CHECK(builtin.hash() == before && builtin.list_count() == 3);

  // The hash is the wake snapshot's FNV-1a.
  uint32_t fnv = 2166136261u;
  for (const char *c = "light.lamp"; *c != '\0'; c++) {
    fnv = (fnv ^ static_cast<uint8_t>(*c)) * 16777619u;
  }
  CHECK(favorites_text_hash("light.lamp") == fnv);
}

static void test_errors() {
  int line = -1;
  CHECK(error_of("#A\nLight light.x\n", &line) == "Line 2: expected Name|entity_id");
  CHECK(line == 2);
  CHECK(error_of("#A\n|light.x\n") == "Line 2: the favorite needs a name before the |");
  CHECK(error_of("#A\nLamp|\n") == "Line 2: the favorite needs an entity ID after the |");
  CHECK(error_of("#A\nLamp|  |x\n") == "Line 2: the favorite needs an entity ID after the |");
  CHECK(error_of("#A\nLamp|Light.Office\n") ==
        "Line 2: Light.Office isn't an entity ID (lower-case letters, digits and _, with one dot)");
  for (const char *bad : {"light", "light.", ".lamp", "light.a.b", "light.a b"}) {
    CHECK(error_of(std::string("#A\nLamp|") + bad + "\n").find("isn't an entity ID") != std::string::npos);
  }
  CHECK(error_of("\n\n#A\nZone|zone.home\n", &line) == "Line 4: zone.home: the remote doesn't support this domain");
  CHECK(line == 4);
  CHECK(error_of("#A\nTV|remote.tv|lg\n").rfind("Line 2: a remote's third field", 0) == 0);
  CHECK(error_of("#A\nTV|remote.tv|a|b|c|d|e|f|g\n").empty());
  CHECK(error_of("#A\nTV|remote.tv|a|b|c|d|e|f|g|hub\n").empty());
  CHECK(!error_of("#A\nTV|remote.tv|a|b|c|d|e|f\n").empty());
  CHECK(error_of("#A\nTV|remote.tv\n").empty());
  CHECK(error_of("#\nLamp|light.x\n") == "Line 1: a list needs a title after #");
  CHECK(error_of("", &line) == "the list has no favorites");
  CHECK(line == 0);
  CHECK(error_of("#A\n#B\n\n") == "the list has no favorites");

  std::string lists;
  for (int i = 0; i < 17; i++) {
    lists += "#L" + std::to_string(i) + "\nx|light.x\n";
  }
  CHECK(error_of(lists, &line) == "Line 33: more than 16 lists");  // the 17th title
  CHECK(line == 33);
  std::string big = "#BIG\n";
  for (int i = 0; i < 65; i++) {
    big += "x|light.l" + std::to_string(i) + "\n";
  }
  CHECK(error_of(big) == "Line 66: more than 64 favorites in BIG");
  CHECK(error_of(std::string(FAVORITES_MAX_BYTES + 1, ' ')) == "the list is 8193 bytes; the remote takes up to 8192");
  CHECK(error_of(std::string("#A\nLa\0mp|light.x\n", 17)) == "Line 2: holds a NUL character");
  FavoriteSet set;
  std::string error;
  CHECK(!set.parse(nullptr, 5, &error));

  // Every domain the remote supports.
  const char *all =
      "#A\na|light.a\nb|switch.a\nc|input_boolean.a\nd|climate.a\ne|water_heater.a\nf|humidifier.a\ng|fan.a\n"
      "h|cover.a\ni|valve.a\nj|lock.a\nk|media_player.a\nl|sensor.a\nm|binary_sensor.a\nn|person.a\n"
      "o|device_tracker.a\np|event.a\nq|automation.a\nr|script.a\ns|scene.a\nt|button.a\nu|input_button.a\n"
      "v|alarm_control_panel.a\nw|weather.a\nx|number.a\ny|input_number.a\nz|select.a\naa|input_select.a\n"
      "ab|vacuum.a\nac|lawn_mower.a\nad|timer.a\nae|remote.a\n";
  CHECK(error_of(all).empty());
}

// Random text never crashes the parser, a failure leaves the set as it was,
// and what it takes comes back the same from its own text.
static void test_random() {
  static const FavoriteEntity A[] = {{"Lamp", "light.lamp"}};
  static const FavoriteList LISTS[] = {make_favorite_list("ONE", A)};
  FavoriteSet keep;
  keep.use(LISTS, 1);
  const uint32_t kept = keep.hash();
  const char alphabet[] = "#|\n\r \tabcxyz._0LIGHTlight.remote.samsung";
  srand(1);
  std::string error;
  for (int iter = 0; iter < 200000; iter++) {
    std::string text;
    for (int i = rand() % 120; i > 0; i--) {
      text += alphabet[rand() % (sizeof(alphabet) - 1)];
    }
    FavoriteSet set;
    set.use(LISTS, 1);
    if (!set.parse(text.data(), text.size(), &error)) {
      CHECK(set.hash() == kept);
      continue;
    }
    std::string canonical = set.text();
    FavoriteSet again;
    CHECK(again.parse(canonical.data(), canonical.size(), &error));
    CHECK(again.hash() == set.hash() && again.text() == canonical);
    int grouped = 0;
    for (int mode = 0; mode < REMOTE_MODE_COUNT; mode++) {
      grouped += set.mode_count(static_cast<RemoteMode>(mode));
    }
    CHECK(grouped <= set.favorite_count());
  }
}

int main() {
  test_parse();
  test_builtin();
  test_errors();
  test_random();
  printf(failures == 0 ? "favorite lists: all passed\n" : "favorite lists: %d failed\n", failures);
  return failures == 0 ? 0 : 1;
}
