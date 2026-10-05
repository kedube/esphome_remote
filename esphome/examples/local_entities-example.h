#pragma once

// Your favorite lists live in Home Assistant, so you can change them without
// rebuilding the firmware: add the template sensor in
// home_assistant/remote_favorites.yaml (see Favorites from Home Assistant in
// the README). Until the remote has those lists it uses these, which can stay
// empty: an empty list is skipped in the menu.
inline constexpr FavoriteList FAVORITE_LISTS[] = {
    {"FAVORITES", nullptr, 0},
};

// To build the lists into the firmware instead, replace FAVORITE_LISTS above
// with lists like these (see Lists in the firmware in the README):
//
// inline constexpr FavoriteEntity LIVING_ROOM_FAVORITES[] = {
//     {"Lamp", "light.living_room_lamp"},
//     {"Thermostat", "climate.main_thermostat"},
//     // TVs and receivers pick up their source list from Home Assistant. For
//     // other media players, list the sources yourself with an optional third
//     // field to get a SOURCE setting; omit it and no SOURCE setting appears.
//     {"Speaker", "media_player.living_room_speaker", "Spotify|Radio|Line In"},
//     // A TV or streaming box's remote takes its command set as the third
//     // field: apple_tv, android_tv, roku, samsung, bravia or philips.
//     {"Apple TV", "remote.living_room_apple_tv", "apple_tv"},
// };
//
// inline constexpr FavoriteEntity KITCHEN_FAVORITES[] = {
//     {"Ceiling Light", "light.kitchen_ceiling"},
//     {"Pasta Timer", "timer.kitchen"},
// };
//
// inline constexpr FavoriteList FAVORITE_LISTS[] = {
//     make_favorite_list("LIVING ROOM", LIVING_ROOM_FAVORITES),
//     make_favorite_list("KITCHEN", KITCHEN_FAVORITES),
// };

// The remote reads its lists from sensor.remote_favorites. Name another
// sensor to give this remote lists of its own, or set it to "" to use only
// the lists above.
// #define FAVORITES_ENTITY "sensor.remote_favorites"

// Lights with a colour temperature get a WARMTH setting. Following it costs
// three subscriptions per light, which a wake spends a little longer syncing;
// uncomment to go without.
// #define LIGHT_WARMTH 0

// Lights that take a colour get a COLOR setting, which costs one subscription
// per light; uncomment to go without.
// #define LIGHT_COLOR 0

// Optional notifications mode. Leave NOTIFICATION_FEED_ENTITY undefined or set it
// to an empty string to hide Notifications from the UI entirely.
#define NOTIFICATION_FEED_ENTITY "sensor.remote_notifications"
#define NOTIFICATION_FEED_ATTRIBUTE "messages"
#define NOTIFICATION_FEED_IDS_ATTRIBUTE "ids"
#define NOTIFICATION_FEED_SEPARATOR "||"
