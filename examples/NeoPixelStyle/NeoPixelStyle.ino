// The classic NeoPixel "strandtest" routines, unchanged except for the type
// and the constructor: HD108NeoPixel has the Adafruit_NeoPixel method names.
// Colors are linear 8-bit exactly as a WS2812 shows them; setBrightness()
// dims by LED current with a 16-bit remainder, so even brightness 10 keeps
// full color resolution.
#include <HD108NeoPixel.h>

constexpr uint16_t kPixels = 60;
HD108NeoPixel strip(kPixels, /*data*/ 23, /*clock*/ 18);

void setup() {
  strip.begin();
  strip.setColorOrder(ColorOrder::BGR);  // datasheet order; some batches are RGB
  strip.setBrightness(40);               // try 10: still smooth, no steps
  strip.show();
}

void loop() {
  colorWipe(strip.Color(255, 0, 0), 30);
  colorWipe(strip.Color(0, 255, 0), 30);
  colorWipe(strip.Color(0, 0, 255), 30);
  theaterChase(strip.Color(127, 127, 127), 50);
  rainbow(10);
}

void colorWipe(uint32_t color, int wait) {
  for (uint16_t i = 0; i < strip.numPixels(); ++i) {
    strip.setPixelColor(i, color);
    strip.show();
    delay(wait);
  }
}

void theaterChase(uint32_t color, int wait) {
  for (int a = 0; a < 10; ++a) {
    for (int b = 0; b < 3; ++b) {
      strip.clear();
      for (uint16_t c = b; c < strip.numPixels(); c += 3) {
        strip.setPixelColor(c, color);
      }
      strip.show();
      delay(wait);
    }
  }
}

void rainbow(int wait) {
  for (long hue = 0; hue < 5 * 65536; hue += 256) {
    for (uint16_t i = 0; i < strip.numPixels(); ++i) {
      const long pixelHue = hue + (i * 65536L / strip.numPixels());
      strip.setPixelColor(i, hsvColor((uint16_t)pixelHue));
    }
    strip.show();
    delay(wait);
  }
}

// 16-bit hue -> packed 8-bit color (full saturation and value), like
// Adafruit_NeoPixel::ColorHSV without the gamma.
uint32_t hsvColor(uint16_t hue) {
  const uint8_t sector = hue / 10923;  // 0..5
  const uint8_t f = (uint8_t)(((uint32_t)(hue % 10923) * 255) / 10923);
  switch (sector) {
    case 0:  return HD108NeoPixel::Color(255, f, 0);
    case 1:  return HD108NeoPixel::Color(255 - f, 255, 0);
    case 2:  return HD108NeoPixel::Color(0, 255, f);
    case 3:  return HD108NeoPixel::Color(0, 255 - f, 255);
    case 4:  return HD108NeoPixel::Color(f, 0, 255);
    default: return HD108NeoPixel::Color(255, 0, 255 - f);
  }
}
