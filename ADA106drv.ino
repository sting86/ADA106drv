#include <ArduinoSTL.h>
#include <vector>
#include <Adafruit_NeoPixel.h>

#define NUM_LEDS 16

#define BRANCH1_LEDS 16
#define BRANCH2_LEDS 12

#define BRANCH1_PIN 14
#define BRANCH2_PIN 2


Adafruit_NeoPixel tree[] = {
  Adafruit_NeoPixel(BRANCH1_LEDS, BRANCH1_PIN, NEO_GRB + NEO_KHZ400),
  Adafruit_NeoPixel(BRANCH2_LEDS, BRANCH2_PIN, NEO_GRB + NEO_KHZ400),
};

#define BRANCHES (sizeof(tree)/sizeof(tree[0]))

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
  uint8_t value;

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

    sortParams () : sortBy(COL_N) {}

    sortParams(ColumnType sortByCol) :  sortBy(sortByCol) {}

    void setSortBy (ColumnType column = COL_N) {
      sortBy = column;
    }
    bool operator() (T a, T b) {
      //TODO: ASC vs DSC
      return a.getAttribute(sortBy) < b.getAttribute(sortBy);
    }

  private:
    ColumnType sortBy;
};

std::vector<LED> treeV;

class Animation {
  public:
    virtual void nextFrame() = 0;
};

class AnimRainbow: public Animation {

  public:
    AnimRainbow(): speed(40) {};
    virtual void nextFrame() {
      byte *c;
      uint16_t i = 0;

      for (std::vector<LED>::iterator it = treeV.begin(); it != treeV.end(); it++) {
        c = this->Wheel(((i * 256 / NUM_LEDS) + frameIdx) & 255);

        it->color = (Adafruit_NeoPixel::Color(c[0], c[1], c[2]));
        i++;
      }
      ++frameIdx;
      delay(speed);
    }

  private:
    uint8_t speed;
    uint8_t frameIdx;
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

};

class AnimFlame: public Animation {

  public:
    AnimFlame(): speed(40) {};
    virtual void nextFrame() {

      for (std::vector<LED>::iterator it = treeV.begin(); it != treeV.end(); it++) {
        it->color = (Adafruit_NeoPixel::Color(g + random(0, flickRand), r + random(0, flickRand), b + random(0, 12)));
      }
      delay(speed);
    }

  private:
    const uint8_t flickRand = 40;
    const uint8_t r = 215;
    const uint8_t g = 56;
    const uint8_t b = 0;
    uint8_t speed;
};


class AnimFire: public Animation {

  public:
    AnimFire(): speed(50) , Sparking(200), Cooling(5) {};
    virtual void nextFrame() {

      static byte heat[NUM_LEDS];
      int cooldown;

      // Step 1.  Cool down every cell a little
      for ( int i = 0; i < NUM_LEDS; i++) {
        cooldown = random(10, (10 + (Cooling * 10) / NUM_LEDS) + 2);

        if (cooldown > heat[i]) {
          heat[i] = 0;
        } else {
          heat[i] = heat[i] - cooldown;
        }
      }

      // Step 2.  Heat from each cell drifts 'up' and diffuses a little
      for ( int k = NUM_LEDS - 1; k >= 2; k--) {
        heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
      }

      // Step 3.  Randomly ignite new 'sparks' near the bottom
      if ( random(255) < Sparking ) {
        int y = random(3);
        heat[y] = heat[y] + random(50, 100);
        //heat[y] = random(160,255);
      }

      // Step 4.  Convert heat to LED colors
      for ( int j = 0; j < NUM_LEDS; j++) {
        setPixelHeatColor(j, heat[j] );
      }

      delay(speed);
    }

    void setPixelHeatColor (int Pixel, byte temperature) {
      // Scale 'heat' down from 0-255 to 0-191
      //byte t192 = round((temperature/255.0)*191);
      byte t192 = (uint8_t)(((uint16_t)temperature * 191) / 255);

      // calculate ramp up from
      byte heatramp = t192 & 0x3F; // 0..63
      heatramp <<= 2; // scale up to 0..252

      if (heatramp < 11) heatramp = 11;

      // figure out which third of the spectrum we're in:
      if ( t192 > 0x80) {                    // hottest
        treeV[Pixel].color = (Adafruit_NeoPixel::Color(255 / 3, 255, heatramp / 10));
        //setPixel(Pixel, 255, 255, heatramp);
      } else if ( t192 > 0x40 ) {            // middle
        treeV[Pixel].color = (Adafruit_NeoPixel::Color(heatramp / 3, 255, 0));
      } else {                               // coolest
        treeV[Pixel].color = (Adafruit_NeoPixel::Color(0, heatramp, 0));
      }
    }
  private:
    uint8_t Cooling;
    uint8_t Sparking;
    uint8_t speed;
};

class AnimWater: public Animation {

  public:
    enum PaletteType {
      PALETTE_BLUE,
      PALETTE_RED
    };
    AnimWater(): speed(50) , mass(127), diversity(25), palette(PALETTE_RED) {};
    virtual void nextFrame() {

      static byte heat[NUM_LEDS];
      int cooldown;

      for (uint8_t i = treeV.size()-1; i>0; i--) {
        treeV[i].value = (uint8_t) ((((uint16_t) treeV[i].value)*1 + ((uint16_t) treeV[i-1].value)*3)/4);
        treeV[i].color = getPalette(treeV[i].value);
      }

      if (diversity > random(0, 254)) {
        mass = random(0, 255);
      }

      treeV[0].value = (uint8_t) ((((uint16_t) treeV[0].value)*5 + (uint16_t) mass)/6);
      treeV[0].color = getPalette(treeV[0].value);

      delay(speed);
    }
  private:   
    uint32_t getPalette (uint8_t hue) {
        switch (palette) {
          case PALETTE_BLUE:
            return getPaletteBlue(hue);
          break;

          case PALETTE_RED:
            return getPaletteFire(hue);
          break;

          default:
            return 20;
        }
        return (Adafruit_NeoPixel::Color(hue, hue/5, 255));
    }

    inline uint32_t getPaletteBlue (uint8_t hue) {
        return (Adafruit_NeoPixel::Color(hue, hue/5, 255));
    }
    
    inline uint32_t getPaletteFire (uint8_t hue) {
//        return (Adafruit_NeoPixel::Color((hue)/3, 255, (hue)/10)); 

        if (hue < 128 ){
          return (Adafruit_NeoPixel::Color(0, 127+hue, 0));
        } else {
          return (Adafruit_NeoPixel::Color((hue-127)/3, 255, (hue-127)/10));
        }
    }
    PaletteType palette;
    uint8_t diversity;
    uint8_t mass;
    uint8_t speed;
};

AnimRainbow rainbow;
AnimFlame flame;
AnimFire fire;
AnimWater water;


void setup() {
  // put your setup code here, to run once:

  treeV.push_back(LED(1, 1, 0, 0));
  treeV.push_back(LED(1, 1, 1, 0));
  treeV.push_back(LED(1, 1, 2, 0));
  treeV.push_back(LED(1, 1, 3, 0));
  treeV.push_back(LED(1, 1, 4, 0));
  treeV.push_back(LED(1, 1, 5, 0));
  treeV.push_back(LED(1, 1, 6, 0));
  treeV.push_back(LED(1, 1, 7, 0));
  treeV.push_back(LED(1, 1, 8, 0));
  treeV.push_back(LED(1, 1, 9, 0));
  treeV.push_back(LED(1, 1, 10, 0));
  treeV.push_back(LED(1, 1, 11, 0));
  treeV.push_back(LED(1, 1, 12, 0));
  treeV.push_back(LED(1, 1, 13, 0));
  treeV.push_back(LED(1, 1, 14, 0));
  treeV.push_back(LED(1, 1, 15, 0));

  for (uint8_t i = 0; i < BRANCHES; ++i) {
    tree[i].begin();
  }
}




void play(Animation &anim) {
  anim.nextFrame();

  for (std::vector<LED>::iterator it = treeV.begin(); it != treeV.end(); it++) {
    tree[it->branchID].setPixelColor(it->n, it->color);
  }

  for (uint8_t i = 0; i < BRANCHES; ++i) {
    tree[i].show();
  }

}

void loop() {
  class sortParams<LED> params;
  // put your main code here, to run repeatedly:`
  //displayFrame();
  std::sort(treeV.begin(), treeV.end(), params);
  play(water);
}
