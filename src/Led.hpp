#ifndef _LED_HPP
#define _LED_HPP

#include <Arduino.h>

#define LED_OFF 0
#define LED_ON 1
#define LED_SLOW_FLASH 2
#define LED_FLASH 3
#define LED_FAST_FLASH 4
#define LED_REDICULOUS_FLASH 5

class Led {

    private:
        byte pinNumber;
        byte state;

        bool onOffState;
        unsigned long onTime[6] = { 0L, 1000L, 500L, 250L, 130L, 25L };
        unsigned long oldTime;

    public:

        Led() {}

        Led(byte pinNumber, byte initialState): pinNumber(pinNumber), state(initialState) {
            oldTime = millis();
            onOffState = false;
        }

        void setState(byte someState) {
            state = someState;
        }

        void tick() {
            if (state < LED_SLOW_FLASH) {
                // We do not flash.
                if (state == LED_OFF) { // The led is switched off.
                    digitalWrite(pinNumber, LOW); // Switch led OFF.
                } else { // The led is lit continuously.
                    digitalWrite(pinNumber, HIGH); // Switch led ON.
                }
            } else {
                // We flash.
                if ((millis() - oldTime) > onTime[state]) {
                    oldTime = millis();
                    onOffState = !onOffState;
                    digitalWrite(pinNumber, onOffState);
                }
            }
        }
};

#endif