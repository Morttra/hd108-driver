#pragma once
#if __has_include(<FastLED.h>)
#include <FastLED.h>

#include "HD108.h"

// FastLED controller for HD108 strips that dims by LED current.
//
// FastLED 3.10+ drives HD108 natively (addLeds<HD108, DATA, CLOCK>), with a
// fixed gamma 2.8, but it pins every pixel's current gain at 31 and applies
// FastLED.setBrightness() by scaling the 8-bit colors before they are
// expanded, so low brightness loses resolution the way it does on APA102.
// This controller instead puts the coarse part of the brightness into the
// HD108's current gain and the remainder into the 16-bit values: dimming
// keeps full color resolution, never flickers, and actually lowers the
// strip's current. Colors stay linear 8-bit as FastLED intends unless
// setGamma(true) expands them through a 2.2 curve. Color correction and
// temperature are honored; temporal dithering is pointless with 16 bits and
// is ignored.
//
// Register it with the pointer form of addLeds:
//
//   CRGB leds[NUM];
//   HD108GainController ctrl(NUM, /*data*/ 23, /*clock*/ 18);
//   FastLED.addLeds(&ctrl, leds, NUM);
class HD108GainController : public CLEDController {
public:
  HD108GainController(uint16_t n, int8_t dataPin, int8_t clockPin,
                      uint32_t spiHz = 10000000UL, SPIClass& spi = SPI)
      : strip_(n, dataPin, clockPin, spiHz, spi) {}

  void init() noexcept override { strip_.begin(); }

  // HD108 extras.
  void setColorOrder(ColorOrder order) { strip_.setColorOrder(order); }
  void setGamma(bool on) { gamma_ = on; }
  HD108Strip& strip() { return strip_; }  // the native driver underneath

protected:
  void showColor(const CRGB& data, int nLeds, uint8_t brightness) noexcept override {
    render(&data, nLeds, brightness, /*stride=*/0);
  }
  void show(const CRGB* data, int nLeds, uint8_t brightness) noexcept override {
    render(data, nLeds, brightness, /*stride=*/1);
  }

private:
  void render(const CRGB* data, int nLeds, uint8_t brightness, int stride) {
    const bool reversed = nLeds < 0;  // FastLED's convention for reversed strips
    uint16_t n = (uint16_t)(reversed ? -nLeds : nLeds);
    if (n > strip_.size()) n = strip_.size();
    const CRGB adj = getAdjustment(255);  // correction + temperature only
    // Brightness: coarse part as current gain, remainder as a 16-bit scale.
    const uint8_t gain = (uint8_t)(((uint16_t)brightness * 31 + 254) / 255);
    const uint32_t num = (uint32_t)brightness * 31;
    const uint32_t den = 255u * (gain ? gain : 1);
    if (gain != lastGain_) {
      strip_.setGlobalGain(gain);
      lastGain_ = gain;
    }
    for (uint16_t i = 0; i < n; ++i) {
      const CRGB& c = data[(reversed ? (n - 1 - i) : i) * stride];
      const uint8_t raw[3] = {c.r, c.g, c.b};
      const uint8_t a[3] = {adj.r, adj.g, adj.b};
      uint16_t v[3];
      for (int k = 0; k < 3; ++k) {
        const uint8_t s = scale8(raw[k], a[k]);
        const uint16_t lin = gamma_ ? HD108Strip::gamma16(s) : (uint16_t)(s * 257u);
        v[k] = (uint16_t)((uint32_t)lin * num / den);
      }
      strip_.setPixel(i, v[0], v[1], v[2]);
    }
    strip_.show();
  }

  HD108Strip strip_;
  uint8_t lastGain_ = 255;  // never a valid gain, forces the first write
  bool gamma_ = false;
};
#else
#error "HD108FastLED.h needs the FastLED library (add fastled/FastLED to lib_deps)"
#endif
