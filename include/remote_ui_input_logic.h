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

// traits: what else the prompt depends on (see selected_entity_traits).
RemoteButtonPrompt describe_remote_button_prompt(
    RemoteMode mode, int action, const std::string &selected_item_state, int selected_alarm_arm_mode,
    uint32_t default_hold_ms, uint32_t extended_hold_ms, const RemoteEntityTraits &traits = {});

// Press state of one of the three action buttons, indexed by action:
// 0 square (primary), 1 settings, 2 circle (play/pause).
struct RemoteHoldButton {
  uint32_t started_at{0};
  int mode{-1};
  bool fired{false};
};

// True while a protected action is being held in the current mode: label is
// its prompt ("HOLD TO LOCK") and progress how far through the hold it is.
bool describe_active_hold(
    RemoteMode mode, uint32_t now, const RemoteHoldButton (&buttons)[3], const std::string &selected_item_state,
    int selected_alarm_arm_mode, uint32_t default_hold_ms, uint32_t extended_hold_ms, std::string &label,
    int &progress, const RemoteEntityTraits &traits = {});
