// First thing to run with a new reel: fills the strip red, green and blue
// in turn (announced on Serial), then runs a single white dot from pixel 0
// in data direction. If the colors you see do not match the log, change
// the setColorOrder() line until they do. The dot shows where pixel 0 is
// and which way the data runs.
#include <HD108.h>

constexpr uint16_t kPixels = 60;
HD108Strip strip(kPixels, /*data*/ 23, /*clock*/ 18, /*SPI Hz*/ 10000000UL);

void setup() {
  Serial.begin(115200);
  strip.begin();
  strip.setColorOrder(ColorOrder::BGR);  // datasheet order; try RGB if wrong
  strip.setGlobalGain(6);                // gentle, this is a bench test
}

void loop() {
  static const char* kNames[] = {"RED", "GREEN", "BLUE"};
  for (int c = 0; c < 3; ++c) {
    Serial.printf("all pixels should now be %s\n", kNames[c]);
    strip.fill(c == 0 ? 30000 : 0, c == 1 ? 30000 : 0, c == 2 ? 30000 : 0);
    strip.show();
    delay(2000);
  }
  Serial.println("white dot running from pixel 0 in data direction");
  for (uint16_t i = 0; i < strip.size(); ++i) {
    strip.clear();
    strip.setPixel(i, 40000, 40000, 40000);
    strip.show();
    delay(40);
  }
}
