#include "framebuffer_web_debug.h"

#if defined(REMOTE_FRAMEBUFFER_WEB_DEBUG) && REMOTE_FRAMEBUFFER_WEB_DEBUG

#define protected public
#include "esphome/components/display/display_buffer.h"
#include "esphome/components/ssd1306_i2c/ssd1306_i2c.h"
#undef protected

#include <string>

#include "esphome/components/web_server_base/web_server_base.h"
#include "esphome/components/web_server_idf/web_server_idf.h"
#include "esphome/core/log.h"

namespace esphome {
namespace remote_debug {

static const char *const TAG = "framebuffer_debug";
static const char *const FRAMEBUFFER_DUMP_PATH = "/debug/framebuffer.pbm";

inline bool framebuffer_pixel_is_on(ssd1306_i2c::I2CSSD1306 *display, int x, int y) {
  const int width = display->get_width();
  const int index = x + (y / 8) * width;
  return (display->buffer_[index] & (1u << (y & 0x07))) != 0;
}

class FramebufferDumpHandler : public web_server_idf::AsyncWebHandler {
 public:
  explicit FramebufferDumpHandler(ssd1306_i2c::I2CSSD1306 *display) : display_(display) {}

  bool canHandle(web_server_idf::AsyncWebServerRequest *request) const override {
    return request->method() == HTTP_GET && request->url() == FRAMEBUFFER_DUMP_PATH;
  }

  void handleRequest(web_server_idf::AsyncWebServerRequest *request) override {
    if (this->display_ == nullptr) {
      request->send(503, "text/plain", "Framebuffer display is unavailable.");
      return;
    }

    const int width = this->display_->get_width();
    const int height = this->display_->get_height();
    auto *response = request->beginResponseStream("image/x-portable-bitmap");
    response->addHeader("Content-Disposition", "attachment; filename=\"oled-framebuffer.pbm\"");
    response->printf("P4\n%d %d\n", width, height);

    for (int y = 0; y < height; y++) {
      const int source_y = height - 1 - y;
      uint8_t packed = 0;
      int bit = 7;
      for (int x = 0; x < width; x++) {
        const int source_x = width - 1 - x;
        if (!framebuffer_pixel_is_on(this->display_, source_x, source_y)) {
          packed |= static_cast<uint8_t>(1u << bit);
        }
        bit--;
        if (bit < 0) {
          response->write(packed);
          packed = 0;
          bit = 7;
        }
      }

      if (bit != 7) {
        response->write(packed);
      }
    }

    request->send(response);
  }

 protected:
  ssd1306_i2c::I2CSSD1306 *display_;
};

}  // namespace remote_debug

void register_framebuffer_web_debug(ssd1306_i2c::I2CSSD1306 *display) {
  static bool registered = false;
  if (registered) {
    return;
  }

  if (display == nullptr) {
    ESP_LOGW(remote_debug::TAG, "Skipping framebuffer dump registration because the display is unavailable.");
    return;
  }

  if (web_server_base::global_web_server_base == nullptr) {
    ESP_LOGW(remote_debug::TAG, "Skipping framebuffer dump registration because the web server is unavailable.");
    return;
  }

  web_server_base::global_web_server_base->add_handler(new remote_debug::FramebufferDumpHandler(display));
  registered = true;
  ESP_LOGI(remote_debug::TAG, "Framebuffer dump enabled at %s", remote_debug::FRAMEBUFFER_DUMP_PATH);
}

}  // namespace esphome

#else

namespace esphome {

void register_framebuffer_web_debug(ssd1306_i2c::I2CSSD1306 *) {}

}  // namespace esphome

#endif
