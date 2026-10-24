// FastLED on HD108 with current-gain dimming: register the controller with
// the pointer form of addLeds, then use FastLED as usual. Needs the FastLED
// library (PlatformIO: lib_deps = fastled/FastLED). FastLED.setBrightness()
// dims by LED current with a 16-bit remainder, so low brightness stays
// smooth — unlike FastLED's built-in HD108 chipset, which keeps the current
// at maximum and scales the 8-bit colors instead.
#include <FastLED.h>
#include <HD108FastLED.h>

constexpr uint16_t kPixels = 60;
CRGB leds[kPixels];
HD108GainController ctrl(kPixels, /*data*/ 23, /*clock*/ 18);

void setup() {
  ctrl.setColorOrder(ColorOrder::BGR);  // datasheet order; some batches are RGB
  FastLED.addLeds(&ctrl, leds, kPixels);
  FastLED.setBrightness(32);
}

void loop() {
  static uint8_t hue = 0;
  fill_rainbow(leds, kPixels, hue++, 255 / kPixels);
  FastLED.show();
  delay(20);
}
