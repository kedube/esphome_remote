#pragma once

static const LightEntity LIGHT_LIST[] = {
    {"Living Room Lamp", "light.living_room_lamp"},
    {"Kitchen Ceiling", "light.kitchen_ceiling"},
    {"Bedroom Lamp", "light.bedroom_lamp"},
};

static const SwitchEntity SWITCH_LIST[] = {
    {"Coffee Maker", "switch.coffee_maker"},
    {"Desk Fan", "switch.desk_fan"},
};

static const ClimateEntity CLIMATE_LIST[] = {
    {"Main Thermostat", "climate.main_thermostat"},
    {"Bedroom Thermostat", "climate.bedroom_thermostat"},
};

static const HumidifierEntity HUMIDIFIER_LIST[] = {
    {"Bedroom Humidifier", "humidifier.bedroom_humidifier"},
    {"Nursery Humidifier", "humidifier.nursery_humidifier"},
};

static const FanEntity FAN_LIST[] = {
    {"Living Room Fan", "fan.living_room_fan"},
    {"Bedroom Fan", "fan.bedroom_fan"},
};

static const CoverEntity COVER_LIST[] = {
    {"Garage Door", "cover.garage_door"},
    {"Living Room Shade", "cover.living_room_shade"},
};

static const LockEntity LOCK_LIST[] = {
    {"Front Door", "lock.front_door"},
};

static const MediaEntity MEDIA_PLAYER_LIST[] = {
    {"Living Room Speaker", "media_player.living_room_speaker"},
    {"Bedroom TV", "media_player.bedroom_tv"},
};

static const SensorEntity SENSOR_LIST[] = {
    {"Indoor Temperature", "sensor.indoor_temperature"},
    {"Front Door", "binary_sensor.front_door"},
};

static const AutomationEntity AUTOMATION_LIST[] = {
    {"Evening Scene", "scene.evening"},
    {"Goodnight Script", "script.goodnight"},
    {"Away Automation", "automation.away_mode"},
};

static const AlarmEntity ALARM_LIST[] = {
    {"Home Alarm", "alarm_control_panel.home"},
};

static const WeatherEntity WEATHER_LIST[] = {
    {"Local Weather", "weather.forecast_home"},
    {"Backup Weather", "weather.backup_provider"},
};

// Optional notifications mode. Leave NOTIFICATION_FEED_ENTITY undefined or set it
// to an empty string to hide Notifications from the UI entirely.
#define NOTIFICATION_FEED_ENTITY "sensor.remote_notifications"
#define NOTIFICATION_FEED_ATTRIBUTE "messages"
#define NOTIFICATION_FEED_IDS_ATTRIBUTE "ids"
#define NOTIFICATION_FEED_SEPARATOR "||"
