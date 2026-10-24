#pragma once
#include <Arduino.h>

#include "HD108.h"

// Adafruit_NeoPixel-style front end for HD108 strips. The method names are
// the familiar ones, so a NeoPixel sketch ports by changing the type and the
// constructor. It is not a subclass of Adafruit_NeoPixel, so libraries that
// demand that exact class will not take it.
//
// What stays the same: colors are 8-bit and linear, exactly as a WS2812
// would show them, so ported sketches look as they did. setGamma(true)
// expands them through a 2.2 curve instead (don't combine with a sketch's
// own gamma32()/gamma8() calls).
//
// What gets better: setBrightness() goes into the HD108's current gain
// plus a 16-bit remainder instead of crushing the 8-bit values, so a strip
// dimmed to 10 % keeps its full color resolution, and dimming is by LED
// current, so it never flickers.
class HD108NeoPixel {
public:
  HD108NeoPixel(uint16_t n, int8_t dataPin, int8_t clockPin,
                uint32_t spiHz = 10000000UL, SPIClass& spi = SPI)
      : strip_(n, dataPin, clockPin, spiHz, spi), n_(n) {}
  ~HD108NeoPixel() { free(px_); }

  bool begin() {
    if (px_ == nullptr) px_ = (uint8_t*)calloc((size_t)n_ * 3, 1);
    return px_ != nullptr && strip_.begin();
  }

  void setPixelColor(uint16_t i, uint8_t r, uint8_t g, uint8_t b) {
    if (i >= n_ || px_ == nullptr) return;
    uint8_t* p = px_ + (size_t)i * 3;
    p[0] = r;
    p[1] = g;
    p[2] = b;
  }
  void setPixelColor(uint16_t i, uint32_t c) {
    setPixelColor(i, (uint8_t)(c >> 16), (uint8_t)(c >> 8), (uint8_t)c);
  }
  uint32_t getPixelColor(uint16_t i) const {
    if (i >= n_ || px_ == nullptr) return 0;
    const uint8_t* p = px_ + (size_t)i * 3;
    return Color(p[0], p[1], p[2]);
  }

  // count = 0 fills to the end, as in Adafruit_NeoPixel.
  void fill(uint32_t c = 0, uint16_t first = 0, uint16_t count = 0) {
    const uint16_t end = count == 0 || first + count > n_ ? n_ : first + count;
    for (uint16_t i = first; i < end; ++i) setPixelColor(i, c);
  }
  void clear() {
    if (px_) memset(px_, 0, (size_t)n_ * 3);
  }

  void setBrightness(uint8_t b) { brightness_ = b; }
  uint8_t getBrightness() const { return brightness_; }
  uint16_t numPixels() const { return n_; }
  static uint32_t Color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
  }

  void show() {
    if (px_ == nullptr) return;
    // Brightness: the coarse part as current gain (ceil of b*31/255), the
    // remainder as a 16-bit scale factor num/den <= 1.
    const uint8_t gain = (uint8_t)(((uint16_t)brightness_ * 31 + 254) / 255);
    const uint32_t num = (uint32_t)brightness_ * 31;
    const uint32_t den = 255u * (gain ? gain : 1);
    if (gain != lastGain_) {
      strip_.setGlobalGain(gain);
      lastGain_ = gain;
    }
    for (uint16_t i = 0; i < n_; ++i) {
      const uint8_t* p = px_ + (size_t)i * 3;
      uint16_t v[3];
      for (int c = 0; c < 3; ++c) {
        const uint16_t lin =
            gamma_ ? HD108Strip::gamma16(p[c]) : (uint16_t)(p[c] * 257u);
        v[c] = (uint16_t)((uint32_t)lin * num / den);
      }
      strip_.setPixel(i, v[0], v[1], v[2]);
    }
    strip_.show();
  }

  // HD108 extras.
  void setColorOrder(ColorOrder order) { strip_.setColorOrder(order); }
  void setGamma(bool on) { gamma_ = on; }
  HD108Strip& strip() { return strip_; }  // the native driver underneath

private:
  HD108Strip strip_;
  uint16_t n_;
  uint8_t* px_ = nullptr;  // 8-bit shadow buffer, like Adafruit_NeoPixel
  uint8_t brightness_ = 255;
  uint8_t lastGain_ = 255;  // never a valid gain, forces the first write
  bool gamma_ = false;
};
