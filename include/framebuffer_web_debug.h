#pragma once

namespace esphome {
namespace ssd1306_i2c {
class I2CSSD1306;
}  // namespace ssd1306_i2c

void register_framebuffer_web_debug(ssd1306_i2c::I2CSSD1306 *display);

}  // namespace esphome
