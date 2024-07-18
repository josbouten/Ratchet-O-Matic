#ifndef _LEDTESTER_HPP
#define _LEDTESTER_HPP


/*
    The sole purpose of this class is to switch the LEDs on and off so that at startup
    of the module the user can see that all are working correctly.

*/
#include <Arduino.h>

extern const byte NR_OF_LEDS;
extern const byte NR_OF_TESTS;

class LedTester {

    private:
        byte leds[NR_OF_LEDS];

    public:
        LedTester(byte _leds[]) {
            for (byte ledCnt = 0; ledCnt < NR_OF_LEDS; ledCnt++) {
                leds[ledCnt] = _leds[ledCnt];
                pinMode(leds[ledCnt], OUTPUT);
            }
        };

    void shortLed(byte nr) {
        digitalWrite(leds[nr], HIGH);
        delay(50);
        digitalWrite(leds[nr], LOW);
        delay(25);
    }

    void _test() {
        shortLed(0);
        shortLed(1);
        shortLed(2);
        shortLed(1);
        shortLed(0);
        shortLed(1);
        shortLed(3);
        shortLed(4);
        shortLed(5);
        shortLed(4);
        shortLed(3);
        shortLed(1);
    }

    void test() {
        for (byte i = 0; i < NR_OF_TESTS; i++) {
            _test();
        }
    }
};
#endif