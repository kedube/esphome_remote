#pragma once

#include <cstdint>
#include <string>

#include "entity_helpers_requests.h"

enum RemoteInputFeedbackTarget {
  REMOTE_INPUT_FEEDBACK_NONE = -1,
  REMOTE_INPUT_FEEDBACK_AUTOMATION = 0,
  REMOTE_INPUT_FEEDBACK_LOCK = 1,
  REMOTE_INPUT_FEEDBACK_COVER = 2,
  REMOTE_INPUT_FEEDBACK_ALARM = 3,
};

struct RemoteButtonPrompt {
  bool requires_long_press{false};
  uint32_t hold_duration_ms{0};
  int feedback_target{REMOTE_INPUT_FEEDBACK_NONE};
  std::string feedback;
};

RemoteButtonPrompt describe_remote_button_prompt(
    RemoteMode mode, int action, const std::string &selected_item_state, int selected_alarm_arm_mode,
    uint32_t default_hold_ms, uint32_t extended_hold_ms);
