#ifndef __MILLIS
#define __MILLIS

#include <Arduino.h>

class MillisDelay {

    private:
        unsigned long delayTime;
        unsigned long oldTime;
        bool alreadyFinished = false;

    public:
        MillisDelay() {}

        MillisDelay(unsigned long delayTime):
            delayTime(delayTime), oldTime(millis()) { }

        void start() {
            oldTime = millis();
            alreadyFinished = false;
        }

        // Return true as soon and as long as the delay has finished.
        bool justFinished() {
            return((millis() - oldTime) > delayTime);
        }

        // Return true one time as soon as the delay has finished
        // but after that for following requests return false until
        // a new start is issued.
        bool finishedOnce() {
            unsigned long diff = millis() - oldTime;
            if (alreadyFinished) {
                return(false);
            } else {
                if (diff > delayTime) {
                    alreadyFinished = true;
                    return(true);
                } else {
                    return(false);
                }
            }
        }

        void set(unsigned long someDelayTime) {
            delayTime = someDelayTime;
        }
};

#endif