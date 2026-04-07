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

RemoteVerificationResult evaluate_lock_verification(
    const std::string &state, const std::string &expected_state, const std::string &success_feedback) {
  RemoteVerificationResult result;
  if (lock_state_matches_expected(state, expected_state)) {
    result.feedback = success_feedback;
    result.complete = true;
    return result;
  }

  result.feedback = lock_operation_feedback_for_state(state);
  result.complete = result.feedback == "JAMMED";
  return result;
}

RemoteVerificationResult evaluate_cover_verification(
    const std::string &state, float position, const std::string &expected_state, const std::string &success_feedback) {
  RemoteVerificationResult result;
  if (cover_state_matches_expected(state, position, expected_state)) {
    result.feedback = success_feedback;
    result.complete = true;
    return result;
  }

  result.feedback = cover_operation_feedback_for_state(state, position);
  return result;
}

RemoteVerificationResult evaluate_alarm_verification(
    const std::string &state, const std::string &expected_state, const std::string &success_feedback,
    const std::string &failure_feedback, bool use_failure_fallback) {
  RemoteVerificationResult result;
  if (alarm_state_matches_expected(state, expected_state)) {
    result.feedback = success_feedback;
    result.complete = true;
    return result;
  }

  result.feedback = alarm_operation_feedback_for_state(state);
  if (result.feedback.empty() && use_failure_fallback) {
    result.feedback = failure_feedback;
    result.complete = true;
  }
  return result;
}

}  // namespace esphome
