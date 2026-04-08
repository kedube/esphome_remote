#include "remote_ui_input_logic.h"

RemoteButtonPrompt describe_remote_button_prompt(
    RemoteMode mode, int action, const std::string &selected_item_state, int selected_alarm_arm_mode,
    uint32_t default_hold_ms, uint32_t extended_hold_ms) {
  RemoteButtonPrompt prompt;

  switch (mode) {
    case REMOTE_MODE_LOCKS:
      if (action == 0 || action == 2) {
        prompt.requires_long_press = true;
        prompt.hold_duration_ms = default_hold_ms;
        prompt.feedback_target = REMOTE_INPUT_FEEDBACK_LOCK;
        prompt.feedback = action == 2 ? "HOLD TO LOCK" : "HOLD TO UNLOCK";
      }
      break;
    case REMOTE_MODE_COVERS:
      if (action == 0 || action == 2) {
        prompt.requires_long_press = true;
        prompt.hold_duration_ms = default_hold_ms;
        prompt.feedback_target = REMOTE_INPUT_FEEDBACK_COVER;
        prompt.feedback = action == 2 ? "HOLD TO CLOSE" : "HOLD TO OPEN";
      }
      break;
    case REMOTE_MODE_AUTOMATION:
      if (action == 0 || action == 2) {
        prompt.requires_long_press = true;
        prompt.hold_duration_ms = default_hold_ms;
        prompt.feedback_target = REMOTE_INPUT_FEEDBACK_AUTOMATION;
        prompt.feedback = "HOLD TO RUN";
      }
      break;
    case REMOTE_MODE_ALARMS:
      if (action == 0 || action == 1 || action == 2) {
        prompt.requires_long_press = true;
        prompt.hold_duration_ms = action == 1 ? extended_hold_ms : default_hold_ms;
        prompt.feedback_target = REMOTE_INPUT_FEEDBACK_ALARM;
        if (action == 1) {
          prompt.feedback = "HOLD TO TRIGGER";
        } else if (action == 2) {
          prompt.feedback = alarm_arm_mode_hold_label(clamp_alarm_arm_mode(selected_alarm_arm_mode));
        } else if (selected_item_state == "unknown") {
          prompt.feedback = "SYNCING";
        } else {
          prompt.feedback = "HOLD TO DISARM";
        }
      }
      break;
    default:
      break;
  }

  return prompt;
}
