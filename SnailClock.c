/*
* A WS2812 light string timer.
*
* For Atmega328
*
* $ make SnailClock DEVICE=atmega328p
* $ avrdude -p atmega328p -c usbasp -U flash:w:SnailClock.hex 
*/


#define MAX_NUM_LIGHTS 30

#define TC1_8MHZ_256PS_1SEC_TICKS 31250
#define SECONDS_RESET_VALUE 59

#define LIGHT_INTENSITY_1 50
#define LIGHT_INTENSITY_2 10

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "light_ws2812/light_ws2812_AVR/Light_WS2812/light_ws2812.h"

/*
/ Modes : SetTime, Counting Down
*/

struct cRGB led[MAX_NUM_LIGHTS];
volatile uint8_t numberOfLights = MAX_NUM_LIGHTS;
volatile uint8_t illuminationNumber = 0;
volatile uint8_t needToUpdateLights = 0;
volatile uint8_t lightIntensity = 50;

volatile uint8_t minutesOnClock = 14;
volatile uint8_t secondsOnClock = SECONDS_RESET_VALUE;
volatile uint8_t timerHasExpired = 0;

void SetLastLightTo(uint8_t red, uint8_t green, uint8_t blue) {
    led[illuminationNumber].r = red;
    led[illuminationNumber].g = green;
    led[illuminationNumber].b = blue;
}

void SetAllLightsTo(uint8_t red, uint8_t green, uint8_t blue) {
    for(uint8_t i = 0; i < numberOfLights; i++) {
        if ( i >= illuminationNumber) {
            led[i].r = red;
            led[i].g = green;
            led[i].b = blue;
        } else {
            led[i].r = 0;
            led[i].g = 0;
            led[i].b = 0;
        }
    }
}

void UpdateLights() {
    ws2812_setleds(led, numberOfLights);
}

/*
    Using 256 prescaler count up to 31,250
*/
void SetupSecondTimer() {
    ICR1 = TC1_8MHZ_256PS_1SEC_TICKS; // TOP Value
    TIMSK1 |= (1 << ICF1); // Enable intterupt when ICR1 value is reached
    TCCR1B |= (1 << WGM13) | (1 << WGM12); // Set to CTC mode
    TCCR1B |= (1 << CS12); // prescaler 256
}

ISR(TIMER1_CAPT_vect) {
    PORTB ^= (1 << PB1);
    if (secondsOnClock > 0) {
        secondsOnClock--;
    } else {
        if (minutesOnClock > 0) {
            minutesOnClock--;
            secondsOnClock = SECONDS_RESET_VALUE;
        } else {
            timerHasExpired = 1;
        }
    }
    // if (secondsOnClock % 2 == 0) {
    //     lightIntensity = LIGHT_INTENSITY_1;
    // } else {
    //     lightIntensity = LIGHT_INTENSITY_2;
    // }
    // needToUpdateLights = 1;
}

int main() {
    // Check Startup State

    // Allocate light data buffer
    SetupSecondTimer();

    // TEST CODE - Setup lights
    DDRB |= (1 << PB1);
    PORTB |= (1 << PB1);
    illuminationNumber = 30;
    SetAllLightsTo(0, 0, 0);
    UpdateLights();
    illuminationNumber = 0;

    sei();

    // while loop
    while(1) {
        //if (needToUpdateLights) {
            illuminationNumber = MAX_NUM_LIGHTS - minutesOnClock - 1;
            uint8_t timerHighBits = TCNT1H >> 2;
            uint8_t timerLowBits = TCNT1L;
            SetAllLightsTo(0, 0, 50);
            SetLastLightTo(0, 50  + timerHighBits, 20);
            UpdateLights();
            needToUpdateLights = 0;
        //}
        _delay_ms(100);
    }
}