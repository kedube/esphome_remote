#pragma once

#include <cstdint>
#include <string>

namespace esphome {

enum RemoteFeedbackTarget {
  REMOTE_FEEDBACK_AUTOMATION = 0,
  REMOTE_FEEDBACK_SWITCH = 1,
  REMOTE_FEEDBACK_LOCK = 2,
  REMOTE_FEEDBACK_COVER = 3,
  REMOTE_FEEDBACK_ALARM = 4,
};

struct RemoteFeedbackState {
  std::string last_automation_feedback;
  std::string last_switch_feedback;
  std::string last_lock_feedback;
  std::string last_cover_feedback;
  std::string last_alarm_feedback;
  uint32_t last_automation_interaction = 0;
  uint32_t last_switch_interaction = 0;
  uint32_t last_lock_interaction = 0;
  uint32_t last_cover_interaction = 0;
  uint32_t last_alarm_interaction = 0;
  bool updated_ui = false;
};

void apply_remote_feedback(
    RemoteFeedbackTarget target, const std::string &feedback, uint32_t now, RemoteFeedbackState &state);

}  // namespace esphome
