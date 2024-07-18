#ifndef _LEDS
#define _LEDS

#include <Arduino.h>
#include <LibPrintf.h>
#include "Debug.hpp"

#define NR_OF_LEDS     3
#define NR_TESTS       5
#define LED_TEST_DELAY 100

class LedCluster { // a group of 3 leds that shiw MULT or DIV mode and the ONE status.

  private:
    byte pinA;
    byte pinB;
    byte pinC;
    byte ledState[NR_OF_LEDS]; // [0 ... NR_OF_LEDS - 1]

  public:

    LedCluster(byte _pinA, byte _pinB, byte _pinC): pinA(_pinA), pinB(_pinB), pinC(_pinC) {
      pinMode(pinA, OUTPUT); // DIV
      pinMode(pinB, OUTPUT); // MULT
      pinMode(pinC, OUTPUT); // ONE
      // Initialize all leds to off.
      showMode(INIT);
    }

    void showMode(byte mode) {
      switch(mode) {
        case INIT:
          ledState[0] = 0;
          ledState[1] = 0;
          ledState[2] = 0;
          break;
        case DIV:
          ledState[0] = 1;
          ledState[1] = 0;
          ledState[2] = 0;
          break;
        case ONE:
          ledState[2] = 1;
          // We leave the other 2 leds as they were.
          break;
        case MULT:
          ledState[0] = 0;
          ledState[1] = 1;
          ledState[2] = 0;
          break;
        default:
          // This should never happen!
          debug_print2("setMode unknown mode: %0x02\n", mode);
      }
      digitalWrite(pinA, ledState[0]);
      digitalWrite(pinB, ledState[1]);
      digitalWrite(pinC, ledState[2]);
    }
  };
#endif