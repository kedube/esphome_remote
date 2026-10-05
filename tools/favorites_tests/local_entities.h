#pragma once

// The local_entities.h the tests build with: the lists the remote uses until
// Home Assistant sends its own, written the ways people write them.
inline constexpr FavoriteEntity ROOM_A[] = {
    {"Lamp", "light.lamp"},
    {"Fan", "fan.ceiling", nullptr},
    // Comments, escapes and non-ASCII names, for the converter.
    {"Caf\xC3\xA9 \"Bar\"", "light.cafe"},
    {"Lamp → 2", "light.arrow"},
};

inline constexpr FavoriteEntity ROOM_B[] = {
    {"TV", "remote.tv", "samsung"},
    {"Speaker", "media_player.speaker", "Spotify|Radio|Line In"},
};

inline constexpr FavoriteList FAVORITE_LISTS[] = {
    make_favorite_list("ROOM A", ROOM_A),
    {"EMPTY", nullptr, 0},  // skipped in the menu
    make_favorite_list("ÉTAGE", ROOM_B),
};
