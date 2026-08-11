#pragma once

#include <string>

#include "entity_helpers_requests.h"
#include "esphome/core/time.h"

namespace esphome {

bool remote_ui_has_dual_climate_target(
    const std::string &selected_item_state, float target_temp_low, float target_temp_high);

// Formats the climate/water-heater target line shown in the detail row:
// "LOW: 68°F   HIGH: 74°F" when dual_target, otherwise "TARGET: 70°F".
void format_climate_target_detail(
    char *buffer, size_t buffer_size, bool dual_target, float low, float high, float single_target,
    const char *temperature_unit);

void populate_remote_info_text(
    int info_index, const ESPTime &time_now, const char *version, const char *device_name,
    const char *friendly_name, bool battery_monitoring_available, int battery_percentage, float battery_voltage,
    std::string &primary_text, std::string &secondary_text);

}  // namespace esphome
