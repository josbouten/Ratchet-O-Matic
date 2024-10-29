#ifndef _LEDS
#define _LEDS

#include <Arduino.h>
#include <LibPrintf.h>
#include "Debug.hpp"
#include "Led.hpp"

#define NR_TESTS       5
class LedCluster { // a group of 3 leds that show MULT or DIV mode and the ONE status.

  private:
    byte pinA;
    byte pinB;
    byte pinC;
    Led ledDiv;
    Led ledMult;
    Led ledOne;

  public:
    LedCluster(byte _pinA, byte _pinB, byte _pinC): pinA(_pinA), pinB(_pinB), pinC(_pinC) {
      ledDiv  = Led(pinA, LED_OFF); // DIV
      ledMult = Led(pinB, LED_OFF); // MULT
      ledOne  = Led(pinC, LED_OFF); // ONE
      // Initialize all leds to off.
      setMode(INIT);
    }

    void setMode(byte mode) {
      switch(mode) {
        case INIT:
          ledDiv.setState(LED_OFF);
          ledMult.setState(LED_OFF);
          ledOne.setState(LED_OFF);
          break;
        case DIV:
          ledDiv.setState(LED_ON);
          ledMult.setState(LED_OFF);
          ledOne.setState(LED_OFF);
          break;
        case ONE:
          ledOne.setState(LED_ON);
          // We leave the other 2 leds as they were.
          break;
        case MULT:
          ledOne.setState(LED_OFF);
          ledDiv.setState(LED_OFF);
          ledMult.setState(LED_ON);
          break;
        case MAX_MULT:
          ledOne.setState(LED_OFF);
          ledDiv.setState(LED_OFF);
          ledMult.setState(LED_SLOW_FLASH);
          // We leave the other 2 leds as they were.
          break;
        default:
          // This should never happen!
          debug_print2("setMode unknown mode: %0x02\n", mode);
      }
    }

    void tick() {
      ledDiv.tick();
      ledMult.tick();
      ledOne.tick();
    }
};

#endif