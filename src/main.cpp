/*
  Ratchet-O-Matic by J.S. Bouten, 2024

  While watching a video by Gary P Hayes using a clock multiplier and playing with a Hydra synth
  at my friend Macross' house I got inspired to make a module that can ratchet (would that be a ratcheteer)?

  This is version 0.1 with one pot for chance and one for mult/div factor, 2 corresponding cv inputs,
  a reset button and reset input, a mult/div toggle button and one output.

  There are 2 modes, 'MULT' is for ratcheting and 'DIV' is for dividing the clockpulses.
  MULT can ratchet the clock by 1, 2, 3, 4 and 5.
  DIV can divide by 1, 2, 3, 4, 5, 6, 7, 8, 9 and 16.
  Feed a clock into the 'IN' jack and connect the 'OUT' with a sound source you want to either
  send that clock to unchanged or send a ratcheted clock or a divided clock.
  The maximum value of the Chance Pot and Chance IN is used to determine the odds that DIV or MULT does its thing.
  The Mode button allows for switching between the MULT and DIV mode. The setting will be stored in EEPROM so that
  the device will start up in the mode it was last used.
  The maximum value of the FREQ Pot and FREQ input is used to determine the number of ratchets or the division factor.
  The reset button and input can be used to reset the divider's counter.

  Note: the BLUE led will light up when the freq is set to 1 so that you can easily see that it is not something else ....
  The YELLOW led lights up when chance level is exceeded.
  The RED led shows the outgoing pulses.
  The GREEN leds signal MULT or DIV mode.

  The hardware accompanying this code can be found at https://github.com/josbouten/Ratchet-O-Matic

  July 15. 2024: v0.1

*/
#include <Arduino.h>

#define DEBUG
#define WRITE_TO_EEPROM
//#define INIT_EEPROM

// When using the device the 1st time, make sure the eeprom is
// filled from address 0:
// 1: define INIT_EEPROM
// 2: compile and run once
// 3: un-define INIT_EEPROM
// 4: compile and run as often as you like.

#include "Debug.hpp"

#include <TimerOne.h>
#include "LibPrintf.h"

#include "OneButton.h"
#include "RandomNumberGenerator.hpp"

#define EXT_CLOCK_IN    2 // This MUST be an intrerrupt enabled input; D2 ==> INT0
#define EXT_RESET_MPU   3 // This MUST be an interrrupt enabled input; D3 ==> INT1
#define TOGGLE_DIV_OR_MULT_MPU 4 // D4: button to toggle between multiply and divide function.
#define CLOCK_OUT       5 // D5
#define LED_CHANCE_MPU  6 // D6
#define LED_DIV_MPU     7 // D7
#define LED_MULT_MPU    8 // D8
#define LED_ONE_MPU     9 // D9

#define CHANCE_IN_MPU  A0 // Signal from chance potentiometer
#define FREQ_IN_MPU    A1 // Signal from frequency CV input.
#define FREQ_POT_MPU   A2 // Signal from frequency potentiometer
#define CHANCE_POT_MPU A3 // Signal from chance CV input.


#define OUT_HIGH true // Set to false when using an arduino output only. But if this is followed by a BJT, this must be inverted!
#define OUT_LOW (!OUT_HIGH)

// Define the number of rising clock signals on the ext clock input we use
// to compute the cycle time (inversely related to bpm) of the clock.
#define NR_OF_CYCLES 5

// Do we want to reset estimating the cycle time, this will take #cycles clock pulses
// each time we receive an external reset signal?
//#define RESTART_CLOCK_SPEED_ESTIMATION_ON_RESET

OneButton button(TOGGLE_DIV_OR_MULT_MPU); // Button has pull up resistor and is LOW when pushed.

#define INIT 0
#define DIV  1
#define ONE  2
#define MULT 3

#define NR_OF_LED_MODES 3
byte modes[NR_OF_LED_MODES] = { DIV, ONE, MULT };
#ifdef DEBUG
  String mode_str[NR_OF_LED_MODES] = { "DIV", "ONE", "MULT" };
#endif

const byte NR_OF_LEDS = 6;
byte allLeds[NR_OF_LEDS] = { LED_DIV_MPU, LED_MULT_MPU, LED_ONE_MPU, LED_CHANCE_MPU, EXT_RESET_MPU, CLOCK_OUT };
const byte NR_OF_TESTS = 2;

#include "LedTester.hpp"
#include "LedCluster.hpp"

LedTester ledTester(allLeds);

LedCluster ledCluster(LED_DIV_MPU, LED_MULT_MPU, LED_ONE_MPU);

LFSR_RandomNumberGenerator *randomNumberGenerator;

#define NR_OF_FRACTIONS 7

#define NR_OF_MULT_POT_VALUES 5
byte potValues4Mult[NR_OF_MULT_POT_VALUES] = { 1, 2, 3, 4, 5 };

#define NR_OF_DIV_POT_VALUES 10
byte potValues4Div[NR_OF_DIV_POT_VALUES] =  { 1, 2, 3, 4, 5, 6, 7, 8, 9, 16 };

int getFraction(int nrOfValues, byte potValues[]) {
  // nrOfValues will be 0 ... NR_OF_FRACTIONS - 1 or 0 ... NR_OF_MULT_POT_VALUES - 1
  int maxVal = max(analogRead(FREQ_POT_MPU), analogRead(FREQ_IN_MPU));
  int index = map(maxVal, 0, 1024, 0, nrOfValues);
  //debug_print4("m: %d, i: %2d v:%2d\n", maxVal, index, potValues[index]);
  return(potValues[index]);
}

typedef struct SettingsObjType {
  volatile byte device_mode; // Either DIV or MULT, but never ONE.
  volatile byte dummy[3];    // Add dummy bytes until total size of struct is integer multiple of sizeof(MARKER)
} SettingsObjType_t ;

SettingsObjType_t settings;

#include <EEPROM.h>
#include "Eeprom.hpp"

Eeprom eeprom;

int getChanceValue(int nrOfValues) {
  // Use the maximum value of the potentiometer and the CV input value to determine the chance.
  int maxValue = max(analogRead(CHANCE_POT_MPU), analogRead(CHANCE_IN_MPU));
  //debug_print3("chance_level: %d, cv value: %d\n", potValue, cvValue);
  return(map(maxValue, 0, 1024, 0, nrOfValues));
}

//
// Vars and functions etc. for Timer1 (16 bits) which is used for a clock signal connected to INT0.
//

// Vars to be used with timer 1.
volatile unsigned long cycleTime = 750000L; // Time in microseconds.
volatile unsigned int newPeriodTime;
volatile bool outState = OUT_HIGH;
volatile int frac = 2;
volatile byte irqCnt = 0;
volatile unsigned long oldTime = micros();
volatile unsigned long thisTime;
volatile unsigned long sumTime = 0L;
volatile byte irqCounter = 0;
#ifdef DEBUG
  volatile bool led_builtin_state = true;
#endif

#define MIN_CHANCE_LEVEL 11
#define MAX_CHANCE_LEVEL 89

#define SEVEN_BITS 7

bool allwaysOn = false;

bool oddsInFavour() {
  // We want the chance level to increase when turning the potentiometer to the right.
  int chanceLevel = getChanceValue(100);
  if (chanceLevel < MIN_CHANCE_LEVEL) {
    digitalWrite(LED_CHANCE_MPU, false);
    return(false);
  }
  if (chanceLevel > MAX_CHANCE_LEVEL) {
    digitalWrite(LED_CHANCE_MPU, true);
    return(true);
  }
  // Generate a random value in the range [MIN_CHANCE_LEVEL, MAX_CHANCE_LEVEL]
  // and check whether this is below the chanceLevel. If so, return TRUE,
  // else return FALSE.
  bool prob = randomNumberGenerator->getRandomNumber(MIN_CHANCE_LEVEL, MAX_CHANCE_LEVEL, SEVEN_BITS) < chanceLevel;
  digitalWrite(LED_CHANCE_MPU, prob);
  return(prob);
}

void timerInterrupt() {
  irqCnt++;
  digitalWrite(CLOCK_OUT, outState);
  outState = !outState;
  if (irqCnt > (2 * frac)) {
    Timer1.stop();
  }
}

void clockISR() { // Will respond to a rising edge on INT0
  // Here we handle the 1st half of the cluster
  // which outputs to CLOCK_OUT_MPU
  Timer1.stop();
  // We measure the cycle time in MICRO seconds.
  thisTime = micros();
  // If there is an IRQ from INT0, then increment the counter.
  irqCounter++;
  sumTime += thisTime - oldTime;
  oldTime = thisTime;
  // We use the mean of several inter interrupt times as the cycle time.
  if (irqCounter > NR_OF_CYCLES) {
    cycleTime = sumTime / irqCounter;
    irqCounter = 0;
    sumTime = 0;
  }

  #ifdef DEBUG
    digitalWrite(LED_BUILTIN, led_builtin_state);
    led_builtin_state = !led_builtin_state;
  #endif

  // Allow a dedicated number of values for frac when in MULT mode or in DIV mode.
  if (settings.device_mode == MULT) {
    frac = getFraction(NR_OF_MULT_POT_VALUES, potValues4Mult);
  } else {
    frac = getFraction(NR_OF_DIV_POT_VALUES, potValues4Div);
  }

  if (frac == 1) {
    ledCluster.showMode(ONE);
  } else {
    ledCluster.showMode(settings.device_mode);
  }
  if (frac == 1) { // We pass the clock pulse unchanged.
      irqCnt = 0;
      // The period time will be in micro seconds.
      Timer1.setPeriod(cycleTime / 2);
      Timer1.start();
      // We start with a high output.
      digitalWrite(CLOCK_OUT, OUT_HIGH);
      // The next state will be LOW.
      outState = OUT_HIGH;
  } else { // For all values of frac > 1
    if (settings.device_mode == MULT) {
      // We are multiplying the clock frequency of the 1st clock signal by starting
      // a fast timer and counting its cycles until we have seen enough. The clock may
      // be multiplied by a factor of 1 or higher.
      irqCnt = 0;
      // We start with a high output.
      digitalWrite(CLOCK_OUT, OUT_HIGH);
      // The next state will be LOW.
      outState = OUT_HIGH;
      if (oddsInFavour()) { // Yes, we can ratchet!
        Timer1.setPeriod(cycleTime / 2 / frac);
      } else {
        Timer1.setPeriod(cycleTime / 2);
      }
      // The period time will be in micro seconds.
      Timer1.start();
    } else { // We are in DIV mode.
      // We are counting external clock pulses to divide their frequency.
      irqCnt++;
      // We leave it up to chance whether we divide or not.
      if (oddsInFavour()) {
        if (irqCnt >= frac) {
          irqCnt = 0;
          outState = OUT_HIGH;
          digitalWrite(CLOCK_OUT, OUT_HIGH);
          return;
        }
      }
      digitalWrite(CLOCK_OUT, OUT_LOW);
    }
  }
}

void resetISR() {
  if (settings.device_mode == DIV) {
    // There must be at least one cycleTime between responses to external reset signals or a button push.
    #ifdef RESTART_CLOCK_SPEED_ESTIMATION_ON_RESET
      sumTime = 0;
      irqCounter = 0;
    #endif
    irqCnt = 0;
    Timer1.stop();
    outState = OUT_LOW;
    digitalWrite(CLOCK_OUT, OUT_LOW);
    debug_print(".");
    // As soon as the next clockISR() occurs, the new output value is set synchronous to the clock
  }
}

void button_click() {
  // If the mult/div button was pressed,
  // toggle the settings.device_mode.
  if (settings.device_mode == DIV) {
    settings.device_mode = MULT;
  } else {
    if (settings.device_mode == MULT) {
      settings.device_mode = DIV;
    }
  }
  if (frac == 1) {
    // Light up an additional led.
    ledCluster.showMode(ONE);
  }
  eeprom.writeSettings();
  ledCluster.showMode(settings.device_mode);
}

void setup() {
  debug_begin(230400);
  debug_print("Begin of Setup()\n");
  // EEPROM
  eeprom = Eeprom(EEPROM.length());
  #ifdef INIT_EEPROM
    eeprom.resetStartAddress();
    settings.device_mode = MULT;
    if (eeprom.write(settings) < 0) {
      debug_print("Error writing data to eeprom. Data does not fit in eeprom.\n");
    } else {
      debug_print2("eeprom read  address after writing: %d\n", eeprom.getReadAddress());
      debug_print2("eeprom write address after writing: %d\n", eeprom.getWriteAddress());
    }
  #else
    if (eeprom.isEmpty()) {
      debug_print("Found empty EEPROM.");
      settings.device_mode = MULT;
      if (eeprom.write(settings) < 0) {
        debug_print("Error writing data to eeprom. Data does not fit in eeprom.\n");
      } else {
        debug_print2("eeprom read  address after writing: %d\n", eeprom.getReadAddress());
        debug_print2("eeprom write address after writing: %d\n", eeprom.getWriteAddress());
        debug_print2("Mode (at setup): %s\n", mode_str[settings.device_mode - 1].c_str());
      }
    } else {
      eeprom.read(&settings);
    }

    // Show that all leds work.
    ledTester.test();

    pinMode(LED_BUILTIN, OUTPUT);

    // Potentiometers
    //
    pinMode(FREQ_IN_MPU, INPUT);
    // The first ADC value is unreliable. Let's get it over with.
    analogRead(FREQ_IN_MPU);

    pinMode(FREQ_POT_MPU, INPUT);
    // The first ADC value is unreliable. Let's get it over with.
    analogRead(FREQ_POT_MPU);

    pinMode(CHANCE_IN_MPU, INPUT);
    // The first ADC value is unreliable. Let's get it over with.
    analogRead(CHANCE_IN_MPU);

    pinMode(CHANCE_POT_MPU, INPUT);
    // The first ADC value is unreliable. Let's get it over with.
    analogRead(CHANCE_POT_MPU);

    pinMode(LED_CHANCE_MPU, OUTPUT);

    // Clock inputs.
    //
    pinMode(EXT_CLOCK_IN, INPUT);  // D2/INT0
    pinMode(EXT_RESET_MPU, INPUT); // D3/INT1

    // Define some Toggle buttons for DIV/MULTIPLY and
    // add some LEDs to show their status.
    //

    // Attach functions to the buttons.
    button.attachClick(button_click);

    // Now set the LEDs according to the defaults.
    ledCluster.showMode(settings.device_mode);
    debug_print2("Setting mode to: %s\n", mode_str[settings.device_mode - 1].c_str());

    // Initialize CycleTime before starting the ISR routine.
    oldTime = micros();

    randomNumberGenerator = new LFSR_RandomNumberGenerator(analogRead(A4)); // Get an unused analog input (electrically floating) as a random seed value.

    // Attach IRQs once all the rest has been initialized
    debug_print2("Attaching interrupt 0 to pin D%d for external clock.\n", EXT_CLOCK_IN);
    attachInterrupt(digitalPinToInterrupt(EXT_CLOCK_IN), clockISR, RISING);
    pinMode(CLOCK_OUT, OUTPUT); // Setting output after setting counter modes as advised by the ATmega321P datasheet.


    debug_print2("Attaching interrupt 1 to pin D%d for external reset.\n", EXT_RESET_MPU);
    attachInterrupt(digitalPinToInterrupt(EXT_RESET_MPU), resetISR, RISING);

    outState = OUT_LOW;
    if (settings.device_mode == MULT) {
      frac = getFraction(NR_OF_MULT_POT_VALUES, potValues4Mult);
    } else {
      frac = getFraction(NR_OF_DIV_POT_VALUES, potValues4Div);
    }
    debug_print2("Frac: %d\n", frac);
    Timer1.initialize(cycleTime / 2 / frac);
    if (settings.device_mode == MULT) {
      Timer1.start();
    } else {
      Timer1.stop();
    }
    digitalWrite(CLOCK_OUT, OUT_HIGH);
    Timer1.attachInterrupt(timerInterrupt);
  #endif
  debug_print("End of Setup()\n");
}

void alive() {
  // One second blink period.
  static unsigned long timeBetweenPotValues = 0;
  unsigned long thisTime = millis();
  if ((thisTime - timeBetweenPotValues) > 500) {
    timeBetweenPotValues = thisTime;
    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  }
}

#ifdef INIT_EEPROM
void loop() {
    delay(1000);
    debug_print("Recompile and upload with undefined INIT_EEPROM\n");
    if ((sizeof(SettingsObjType) % sizeof(MARKER)) != 0) {
      debug_print("Error! sizeof(SettingsObjType) should be integer multiple of sizeof(MARKER)!\n");
    }
  }
#else
  void loop() {
    // Show that we are alive.
    alive();
    // Respond to button clicks.
    button.tick();
    eeprom.tick();
  }
#endif