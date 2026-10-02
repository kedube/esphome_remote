#pragma once

#include <string>

#include "entity_helpers_requests.h"
#include "esphome/core/time.h"

namespace esphome {

void populate_remote_info_text(
    int info_index, const ESPTime &time_now, const char *version, const char *device_name,
    const char *friendly_name, bool battery_monitoring_available, int battery_percentage, float battery_voltage,
    std::string &primary_text, std::string &secondary_text);

}  // namespace esphome
