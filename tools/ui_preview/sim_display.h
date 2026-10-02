#pragma once
// Host stand-in for the SH1106: ESPHome's own DisplayBuffer (clipping,
// primitives, font rendering) drawing into a 128x64 1-bit frame.
#include <cstdio>
#include <cstring>

#include "esphome/components/display/display_buffer.h"

class SimDisplay : public esphome::display::DisplayBuffer {
 public:
  uint8_t px[64][128];
  long pixel_calls = 0;

  SimDisplay() { memset(px, 0, sizeof(px)); }

  void draw_pixel_at(int x, int y, esphome::Color color) override {
    pixel_calls++;
    DisplayBuffer::draw_pixel_at(x, y, color);
  }
  void fill(esphome::Color color) override { memset(px, color.is_on() ? 1 : 0, sizeof(px)); }
  void update() override {}
  esphome::display::DisplayType get_display_type() override { return esphome::display::DISPLAY_TYPE_BINARY; }
  int get_width_internal() override { return 128; }
  int get_height_internal() override { return 64; }

  int lit_pixels() const {
    int n = 0;
    for (auto &row : px) {
      for (uint8_t v : row) {
        n += v;
      }
    }
    return n;
  }

  // Plain PBM, the same format as the on-device framebuffer download.
  bool save_pbm(const char *path) const {
    FILE *f = fopen(path, "wb");
    if (f == nullptr) {
      return false;
    }
    fprintf(f, "P1\n128 64\n");
    for (auto &row : px) {
      for (uint8_t v : row) {
        fputc(v ? '1' : '0', f);
      }
      fputc('\n', f);
    }
    fclose(f);
    return true;
  }

 protected:
  void draw_absolute_pixel_internal(int x, int y, esphome::Color color) override {
    if (x >= 0 && x < 128 && y >= 0 && y < 64) {
      px[y][x] = color.is_on() ? 1 : 0;
    }
  }
};
