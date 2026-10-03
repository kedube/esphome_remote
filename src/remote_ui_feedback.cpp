#include "remote_ui_feedback.h"

#include "entity_helpers_requests.h"

namespace esphome {

void apply_remote_feedback(
    RemoteFeedbackTarget target, const std::string &feedback, uint32_t now, RemoteFeedbackState &state) {
  switch (target) {
    case REMOTE_FEEDBACK_AUTOMATION:
      state.last_automation_feedback = feedback;
      state.last_automation_interaction = now;
      break;
    case REMOTE_FEEDBACK_SWITCH:
      state.last_switch_feedback = feedback;
      state.last_switch_interaction = now;
      break;
    case REMOTE_FEEDBACK_LOCK:
      state.last_lock_feedback = feedback;
      state.last_lock_interaction = now;
      break;
    case REMOTE_FEEDBACK_COVER:
      state.last_cover_feedback = feedback;
      state.last_cover_interaction = now;
      break;
    case REMOTE_FEEDBACK_ALARM:
      state.last_alarm_feedback = feedback;
      state.last_alarm_interaction = now;
      break;
  }
  state.updated_ui = true;
}

}  // namespace esphome
