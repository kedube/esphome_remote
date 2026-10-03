#include "remote_ui_input_logic.h"

RemoteButtonPrompt describe_remote_button_prompt(
    RemoteMode mode, int action, const std::string &selected_item_state, int selected_alarm_arm_mode,
    uint32_t default_hold_ms, uint32_t extended_hold_ms, const RemoteEntityTraits &traits) {
  RemoteButtonPrompt prompt;
  const int alarm_features = traits.alarm_features;

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
      // Square (action 0) closes and circle (action 2) opens, matching the
      // off/on split every other mode uses. While the cover moves, either one
      // stops it straight away: stopping is always safe.
      if ((action == 0 || action == 2) && !traits.cover_stoppable) {
        prompt.requires_long_press = true;
        prompt.hold_duration_ms = default_hold_ms;
        prompt.feedback_target = REMOTE_INPUT_FEEDBACK_COVER;
        prompt.feedback = action == 2 ? "HOLD TO OPEN" : "HOLD TO CLOSE";
      }
      break;
    case REMOTE_MODE_AUTOMATION:
      // Only circle (action 2) runs the automation.
      if (action == 2) {
        prompt.requires_long_press = true;
        prompt.hold_duration_ms = default_hold_ms;
        prompt.feedback_target = REMOTE_INPUT_FEEDBACK_AUTOMATION;
        prompt.feedback = traits.automation_kind == AUTOMATION_KIND_BUTTON ? "HOLD TO PRESS" : "HOLD TO RUN";
      }
      break;
    case REMOTE_MODE_ALARMS:
      // Holding Settings triggers the alarm, on a panel that can be triggered;
      // on any other a tap or hold just cycles settings.
      if (action == 0 || action == 2 || (action == 1 && alarm_trigger_supported(alarm_features))) {
        prompt.requires_long_press = true;
        prompt.hold_duration_ms = action == 1 ? extended_hold_ms : default_hold_ms;
        prompt.feedback_target = REMOTE_INPUT_FEEDBACK_ALARM;
        if (action == 1) {
          prompt.feedback = "HOLD TO TRIGGER";
        } else if (action == 2) {
          prompt.feedback = alarm_arm_mode_hold_label(alarm_effective_arm_mode(selected_alarm_arm_mode, alarm_features));
        } else if (selected_item_state.empty() || selected_item_state == "unavailable") {
          prompt.feedback = missing_state_word(selected_item_state);
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

bool describe_active_hold(
    RemoteMode mode, uint32_t now, const RemoteHoldButton (&buttons)[3], const std::string &selected_item_state,
    int selected_alarm_arm_mode, uint32_t default_hold_ms, uint32_t extended_hold_ms, std::string &label,
    int &progress, const RemoteEntityTraits &traits) {
  for (int action = 0; action < 3; action++) {
    const RemoteHoldButton &button = buttons[action];
    if (button.started_at == 0 || button.fired || button.mode != static_cast<int>(mode)) {
      continue;
    }
    RemoteButtonPrompt prompt = describe_remote_button_prompt(
        mode, action, selected_item_state, selected_alarm_arm_mode, default_hold_ms, extended_hold_ms, traits);
    if (!prompt.requires_long_press || prompt.hold_duration_ms == 0) {
      continue;
    }
    uint32_t held = now - button.started_at;
    progress = held >= prompt.hold_duration_ms ? 100 : static_cast<int>(held * 100 / prompt.hold_duration_ms);
    label = prompt.feedback;
    return true;
  }
  return false;
}
