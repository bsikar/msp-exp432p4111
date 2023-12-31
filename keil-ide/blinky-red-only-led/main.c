
// This program will blink a red LED

#include "msp.h"
#include <stdint.h>

#define RED         0x0001
#define ENABLE      0x0000
#define RED_OFF     0x00FE
#define DEVELOPMENT (WDT_A_CTL_PW | WDT_A_CTL_HOLD)
#define DELAY       0x30D40

int main(void) {
  WDT_A->CTL = DEVELOPMENT; // Disables some security features
  P1->SEL0   = ENABLE;      // Enables the pins to the outside world
  P1->SEL1   = ENABLE;      // Enables the pins to the outside world
  P1->DIR    = RED;         // Make a pin an output
  volatile uint_fast16_t x;
  while (1) {
    for (x = 0; x < DELAY; x++) {}
    P1->OUT = RED; // Turn red LED light on
    for (x = 0; x < DELAY; x++) {}
    P1->OUT = RED_OFF; // Turn off the red LED light
  }
}
