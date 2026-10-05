// Checks that the lists favorites_to_home_assistant.py writes are the lists
// the firmware builds from the same local_entities.h. Run by run.py with the
// lists text it wrote; with --parse, only that the remote takes the text.

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

#include "favorites_lists.h"
#include "local_entities.h"

int main(int argc, char **argv) {
  bool parse_only = argc == 3 && std::string(argv[1]) == "--parse";
  if (argc != 2 && !parse_only) {
    fprintf(stderr, "usage: roundtrip [--parse] LISTS_TEXT_FILE\n");
    return 2;
  }
  std::ifstream in(argv[argc - 1], std::ios::binary);
  std::stringstream buffer;
  buffer << in.rdbuf();
  const std::string text = buffer.str();

  FavoriteSet builtin;
  builtin.use(FAVORITE_LISTS, sizeof(FAVORITE_LISTS) / sizeof(FAVORITE_LISTS[0]));
  FavoriteSet converted;
  std::string error;
  if (!converted.parse(text.data(), text.size(), &error)) {
    printf("%s: FAIL, the remote doesn't take the lists: %s\n", parse_only ? "lists" : "converter round trip",
           error.c_str());
    return 1;
  }
  if (parse_only) {
    return 0;
  }
  if (converted.text() != builtin.text()) {
    printf("converter round trip: FAIL\n--- firmware\n%s--- converted\n%s", builtin.text().c_str(),
           converted.text().c_str());
    return 1;
  }
  printf("converter round trip: all passed\n");
  return 0;
}
