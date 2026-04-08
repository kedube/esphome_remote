#pragma once

static const FavoriteEntity LIVING_ROOM_FAVORITES[] = {
    {"Lamp", "light.living_room_lamp"},
    {"Thermostat", "climate.main_thermostat"},
    {"Front Door", "lock.front_door"},
    {"Speaker", "media_player.living_room_speaker"},
    {"Home Alarm", "alarm_control_panel.home"},
};

static const FavoriteEntity KITCHEN_FAVORITES[] = {
    {"Ceiling Light", "light.kitchen_ceiling"},
    {"Coffee Maker", "switch.coffee_maker"},
    {"Garage Door", "cover.garage_door"},
    {"Indoor Temperature", "sensor.indoor_temperature"},
    {"Local Weather", "weather.forecast_home"},
    {"Evening Scene", "scene.evening"},
};

static const FavoriteList FAVORITE_LISTS[] = {
    make_favorite_list("LIVING ROOM", LIVING_ROOM_FAVORITES),
    make_favorite_list("KITCHEN", KITCHEN_FAVORITES),
};

// Optional notifications mode. Leave NOTIFICATION_FEED_ENTITY undefined or set it
// to an empty string to hide Notifications from the UI entirely.
#define NOTIFICATION_FEED_ENTITY "sensor.remote_notifications"
#define NOTIFICATION_FEED_ATTRIBUTE "messages"
#define NOTIFICATION_FEED_IDS_ATTRIBUTE "ids"
#define NOTIFICATION_FEED_SEPARATOR "||"
