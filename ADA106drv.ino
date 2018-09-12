#include <ArduinoSTL.h>
#include <vector>
#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 16

#define BRANCH1_LEDS 16
#define BRANCH2_LEDS 12

#define BRANCH1_PIN 14
#define BRANCH2_PIN 2

Adafruit_NeoPixel branch1 = Adafruit_NeoPixel(NUM_LEDS, 14, NEO_GRB + NEO_KHZ400);
Adafruit_NeoPixel branch2 = Adafruit_NeoPixel(10, 2, NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel tree[] = {
  Adafruit_NeoPixel(BRANCH1_LEDS, BRANCH1_PIN, NEO_GRB + NEO_KHZ400),
  Adafruit_NeoPixel(BRANCH2_LEDS, BRANCH2_PIN, NEO_GRB + NEO_KHZ400),
};

enum ColumnType {
  COL_X,
  COL_Y,
  COL_N,
  COL_BID,
};

struct LED {
  uint8_t x;
  uint8_t y;
  uint8_t n;
  uint8_t branchID;
  uint32_t color;

  uint8_t getAttribute(ColumnType name) {
    switch (name) {
      case COL_X:
        return x;
        break;

      case COL_Y:
        return y;
        break;

      case COL_N:
        return n;
        break;

      case COL_BID:
        return branchID;
        break;

      default:
        return 0x0;
    }
  }

  LED(uint8_t newX, uint8_t newY, uint8_t newN, uint8_t newBID): x(newX), y(newY), n(newN), branchID(newBID) {};
};

template<class T>
class sortParams {
  public:

    void setSortBy (ColumnType column = COL_N) {
      sortBy = column;
    }
    bool operator() (T a, T b) {
      return a.getAttribute(sortBy) > b.getAttribute(sortBy);
    }

  private:
    ColumnType sortBy;
};

std::vector<LED> dupa;

void setup() {
  // put your setup code here, to run once:

  dupa.push_back(LED(1, 1, 1, 0));
  branch1.begin();

}

void rainbowCycle(int SpeedDelay) {
  byte *c;
  uint16_t i, j;

  for (j = 0; j < 256 * 2; j++) { // 5 cycles of all colors on wheel
    for (i = 0; i < NUM_LEDS; i++) {
      c = Wheel(((i * 256 / NUM_LEDS) + j) & 255);
      branch1.setPixelColor(i, *c, *(c + 1), *(c + 2));
    }
    branch1.show();
    delay(SpeedDelay);
  }
}

byte * Wheel(byte WheelPos) {
  static byte c[3];

  if (WheelPos < 85) {
    c[0] = WheelPos * 3;
    c[1] = 255 - WheelPos * 3;
    c[2] = 0;
  } else if (WheelPos < 170) {
    WheelPos -= 85;
    c[0] = 255 - WheelPos * 3;
    c[1] = 0;
    c[2] = WheelPos * 3;
  } else {
    WheelPos -= 170;
    c[0] = 0;
    c[1] = WheelPos * 3;
    c[2] = 255 - WheelPos * 3;
  }

  return c;
}

void displayFrame() {
  uint8_t i = 0;

  static uint8_t R = 0, G = 0, B = 0;

  for (; i < 16; ++i) {
    branch1.setPixelColor(i, Adafruit_NeoPixel::Color(R, G, B)); // Moderately bright green color.

  }

  branch1.show(); // This sends the updated pixel color to the hardware.

  delay(10); // Delay for a period of time (in milliseconds).

  R += 10;
  if (R > 245) G += 10;
  if (G > 245) B += 10;
}

void loop() {
  class sortParams<LED> params;
  // put your main code here, to run repeatedly:`
  //displayFrame();
  std::sort(dupa.begin(), dupa.end(), params);
  rainbowCycle(40);
}
