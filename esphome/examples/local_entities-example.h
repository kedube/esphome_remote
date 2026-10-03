#pragma once

inline constexpr FavoriteEntity LIVING_ROOM_FAVORITES[] = {
    {"Lamp", "light.living_room_lamp"},
    {"Thermostat", "climate.main_thermostat"},
    {"Front Door", "lock.front_door"},
    // TVs and receivers pick up their source list from Home Assistant. For other
    // media players, list the sources yourself with an optional third field to
    // get a SOURCE setting on the remote; omit it and no SOURCE setting appears.
    {"Speaker", "media_player.living_room_speaker", "Spotify|Radio|Line In"},
    {"Home Alarm", "alarm_control_panel.home"},
};

inline constexpr FavoriteEntity KITCHEN_FAVORITES[] = {
    {"Ceiling Light", "light.kitchen_ceiling"},
    {"Coffee Maker", "switch.coffee_maker"},
    {"Garage Door", "cover.garage_door"},
    {"Indoor Temperature", "sensor.indoor_temperature"},
    {"Local Weather", "weather.forecast_home"},
    {"Evening Scene", "scene.evening"},
    {"Pasta Timer", "timer.kitchen"},
};

// Helpers and household devices mix in the same way.
inline constexpr FavoriteEntity HOUSE_FAVORITES[] = {
    {"Guest Mode", "input_boolean.guest_mode"},
    {"TV Power", "button.living_room_tv_power"},
    {"Speaker Bass", "number.living_room_speaker_bass"},
    {"House Mode", "input_select.house_mode"},
    {"Robot Vacuum", "vacuum.robot"},
    {"Garden Water", "valve.garden"},
    {"Alex", "person.alex"},
};

inline constexpr FavoriteList FAVORITE_LISTS[] = {
    make_favorite_list("LIVING ROOM", LIVING_ROOM_FAVORITES),
    make_favorite_list("KITCHEN", KITCHEN_FAVORITES),
    make_favorite_list("HOUSE", HOUSE_FAVORITES),
};

// Lights with a colour temperature get a WARMTH setting. Following it costs
// three subscriptions per light, which a wake spends a little longer syncing;
// uncomment to go without.
// #define LIGHT_WARMTH 0

// Optional notifications mode. Leave NOTIFICATION_FEED_ENTITY undefined or set it
// to an empty string to hide Notifications from the UI entirely.
#define NOTIFICATION_FEED_ENTITY "sensor.remote_notifications"
#define NOTIFICATION_FEED_ATTRIBUTE "messages"
#define NOTIFICATION_FEED_IDS_ATTRIBUTE "ids"
#define NOTIFICATION_FEED_SEPARATOR "||"
