#pragma once

static const FavoriteEntity MAIN_FAVORITES[] = {
    {"Living Room Lamp", "light.living_room_lamp"},
    {"Main Thermostat", "climate.main_thermostat"},
    {"Front Door", "lock.front_door"},
    {"Living Room Speaker", "media_player.living_room_speaker"},
    {"Home Alarm", "alarm_control_panel.home"},
};

static const FavoriteEntity SECONDARY_FAVORITES[] = {
    {"Kitchen Ceiling", "light.kitchen_ceiling"},
    {"Coffee Maker", "switch.coffee_maker"},
    {"Garage Door", "cover.garage_door"},
    {"Indoor Temperature", "sensor.indoor_temperature"},
    {"Local Weather", "weather.forecast_home"},
    {"Evening Scene", "scene.evening"},
};

static const FavoriteList FAVORITE_LISTS[] = {
    {"MAIN", MAIN_FAVORITES, sizeof(MAIN_FAVORITES) / sizeof(MAIN_FAVORITES[0])},
    {"SECONDARY", SECONDARY_FAVORITES, sizeof(SECONDARY_FAVORITES) / sizeof(SECONDARY_FAVORITES[0])},
};

// Optional notifications mode. Leave NOTIFICATION_FEED_ENTITY undefined or set it
// to an empty string to hide Notifications from the UI entirely.
#define NOTIFICATION_FEED_ENTITY "sensor.remote_notifications"
#define NOTIFICATION_FEED_ATTRIBUTE "messages"
#define NOTIFICATION_FEED_IDS_ATTRIBUTE "ids"
#define NOTIFICATION_FEED_SEPARATOR "||"
