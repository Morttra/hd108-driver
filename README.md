# HD108 driver for ESP32

A small, dependency-free driver for **HD108** addressable RGB LEDs on the
ESP32, using hardware SPI. HD108 is an APA102/SK9822-style two-wire part
(data + clock) with two things the 8-bit strips lack: **16 bits per color
channel** (65 536 gray levels at ~28 kHz PWM) and a **5-bit constant-current
gain per pixel** (0–31 steps of ~17 mA). Deep dims stay smooth, and
brightness can be set by current instead of by throwing away PWM
resolution.

Written for a 680-LED lounge lamp (the `hd108-lounge-lamp` firmware). The
driver has no dependency on it and can be dropped into any PlatformIO or
Arduino ESP32 project. Two adapters let existing code use it: a
NeoPixel-style class with the Adafruit_NeoPixel method names, and a FastLED
controller.

## Why not FastLED or NeoPixel?

Adafruit_NeoPixel is WS2812-only. FastLED 3.10 added HD108 support
(`addLeds<HD108, DATA_PIN, CLOCK_PIN>`): 16-bit output behind a fixed
gamma 2.8, which is a fine way to drive the chip from 8-bit colors. What it
does not do is use the current gain. Every pixel's gain is pinned at 31,
and `FastLED.setBrightness()` scales the 8-bit colors before they are
expanded, so at a typical indoor brightness of 10 % you are back to about
25 usable levels per channel, the same cliff as on APA102, with the strip
drawing full current for whatever it shows.

This driver works in 16 bits natively and treats the gain as the
brightness control it was designed to be: dimming happens by LED current,
which never flickers and costs no color resolution. The adapters below keep
the 8-bit APIs that NeoPixel and FastLED code is written against, but route
brightness the same way.

## Getting started

1. Wire data and clock through ~33 Ω resistors to two ESP32 pins (the
   defaults below are VSPI, GPIO 23 data and GPIO 18 clock), common ground,
   5 V to the strip from a supply sized for ~0.26 W per LED at full white.
2. Flash `examples/ColorOrderTest` and watch the serial monitor: it fills
   red, green and blue in turn, then runs a white dot from pixel 0. If the
   colors do not match the log, change the color order. The dot shows
   where the strip starts and which way the data runs.
3. Pick an API: the native one below, `HD108NeoPixel` for NeoPixel-style
   code, or `HD108GainController` for FastLED.

## Wire format

```
start frame : 128 zero bits
per pixel   : 64 bits, MSB first
              1 | gain(5) | gain(5) | gain(5) | c0(16) | c1(16) | c2(16)
end frame   : >= 64 one bits, plus one extra clock edge per two pixels
```

The three gain fields set the constant current of the three outputs; the
16-bit fields are PWM gray levels. Each HD108 delays the regenerated clock
by half a period, which is why the end frame grows with the pixel count:
without the extra clocks the last pixels never latch.

The datasheet lists the color fields as B,G,R, but production batches
differ (the strips this was written for turned out to be R,G,B), so the
order is configurable.

## Usage

The strip class is `HD108Strip` (the bare name `HD108` belongs to FastLED's
chipset list since 3.10; the two coexist).

```cpp
#include <HD108.h>

HD108Strip strip(340, /*data*/ 23, /*clock*/ 18, /*SPI Hz*/ 10000000UL);

void setup() {
  strip.begin();
  strip.setColorOrder(ColorOrder::RGB);
  strip.setGlobalGain(10);               // 10/31 of full current
}

void loop() {
  strip.setPixel(0, 65535, 30000, 5000); // linear 16-bit values
  strip.setPixel8(1, 255, 147, 41);      // 8-bit sRGB-ish, gamma-expanded
  strip.show();
}
```

A second strip on the other bus:

```cpp
SPIClass hspi(HSPI);
HD108Strip up(340, /*data*/ 13, /*clock*/ 14, 10000000UL, hspi);
```

## API

| Call | What it does |
|---|---|
| `HD108Strip(n, dataPin, clockPin, spiHz = 10 MHz, spi = SPI)` | n pixels on the given pins; `spi` picks the bus (VSPI by default) |
| `begin()` / `end()` | allocate the wire buffer and claim the bus; returns false if allocation fails |
| `setColorOrder(ColorOrder)` | BGR, RGB, GRB, BRG, RBG or GBR on the wire |
| `setOffset(int16_t)` | rotate the ring: logical pixel 0 maps to physical pixel `offset` (may be negative) |
| `setGlobalGain(0..31)` / `globalGain()` | current gain for every pixel — a flicker-free master brightness |
| `setPixelGain(i, 0..31)` | per-pixel current gain; `setGlobalGain()` resets all pixels |
| `setPixel(i, r, g, b)` | linear 16-bit values |
| `setPixel8(i, r, g, b)` | 8-bit values through a 2.2 gamma table |
| `getPixel(i, r, g, b)` | read back the 16-bit values |
| `fill(r, g, b)` / `clear()` | every pixel |
| `show()` | one blocking SPI transfer of the whole buffer |
| `size()` | pixel count |
| `HD108Strip::gamma16(v)` | the 8-bit → 16-bit gamma expansion |

`ColorUtil.h` adds `hsv2rgb16(h, s, v, r, g, b)` (all 0..65535, output is
linear) and `lerp16(a, b, frac)`.

## Adapters

### NeoPixel-style: `HD108NeoPixel.h`

The Adafruit_NeoPixel method names on an HD108: `begin`, `setPixelColor`
(both forms), `getPixelColor`, `fill`, `clear`, `setBrightness`,
`getBrightness`, `numPixels`, `Color`, `show`. A strandtest-style sketch
ports by changing the type and the constructor
(`examples/NeoPixelStyle`). It is not a subclass of Adafruit_NeoPixel, so
libraries that demand that exact class will not accept it.

```cpp
#include <HD108NeoPixel.h>
HD108NeoPixel strip(60, /*data*/ 23, /*clock*/ 18);
strip.begin();
strip.setBrightness(10);                 // still smooth: current gain + 16-bit remainder
strip.setPixelColor(0, strip.Color(255, 80, 0));
strip.show();
```

### FastLED: `HD108FastLED.h`

`HD108GainController` is a `CLEDController` for FastLED's pointer form of
`addLeds`; after that every FastLED effect runs on the strip
(`examples/FastLEDDemo`). Unlike FastLED's built-in `HD108` chipset it
dims through the current gain, keeps colors linear unless asked otherwise,
and takes its pins at run time. Needs the FastLED library in your project;
the header errors out if it is missing.

```cpp
#include <FastLED.h>
#include <HD108FastLED.h>
CRGB leds[60];
HD108GainController ctrl(60, /*data*/ 23, /*clock*/ 18);
FastLED.addLeds(&ctrl, leds, 60);
FastLED.setBrightness(32);
```

Color correction and temperature are honored; temporal dithering is
pointless with 16 bits and is ignored.

### What the adapters do with brightness

Both keep colors linear 8-bit by default, exactly as WS2812 or FastLED
would show them, so ported code looks the same. `setGamma(true)` on either
adapter expands colors through the 2.2 curve instead; don't combine it
with a sketch's own `gamma32()` calls.

Brightness is where they improve on the originals. The coarse part goes
into the HD108's current gain (0–31) and the remainder into the 16-bit
values, so `setBrightness(10)` still leaves thousands of levels per
channel instead of about ten, and the dimming is by LED current, which
never flickers.

## Examples

| Sketch | Shows |
|---|---|
| `examples/ColorOrderTest` | the first run with a new reel: color order and data direction |
| `examples/Rainbow` | the native 16-bit API with `hsv2rgb16` |
| `examples/NeoPixelStyle` | Adafruit strandtest routines on `HD108NeoPixel` |
| `examples/FastLEDDemo` | FastLED through `HD108GainController` |

A GitHub Actions workflow (`.github/workflows/build.yml`) compiles all
four on every push.

## Notes

- `show()` blocks for ~2.2 ms per 340 pixels at 10 MHz, so >60 fps leaves
  plenty of loop time for WiFi and the rest.
- HD108 accepts up to 32 MHz. 10 MHz is a safe default over real wiring at
  3.3 V logic into a 5 V strip. Put ~33 Ω in series with data and clock
  near the first LED; add a level shifter (74AHCT125 or similar) if the
  strip glitches at higher clocks.
- Values given to `setPixel()` are linear light. HSV hues from
  `hsv2rgb16()` are too, which reads differently from sRGB intuition: a
  little green mixed into red already looks yellow, and a little blue
  looks pink.
- ESP32 only: the driver relies on `SPIClass::begin(sck, miso, mosi, ss)`
  and `SPIClass::writeBytes()`. Porting means replacing those three calls
  in `HD108.cpp`.
- Memory: one preassembled buffer of 16 + 8·n + end-frame bytes (~2.8 KB
  for 340 pixels).

## License

MIT, see [LICENSE](LICENSE).
