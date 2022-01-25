/*
* A WS2812 light string timer.
*
* For Atmega328
*
* Build & Flash
* $ make
* $ make flash
*/


#define MAX_NUM_LIGHTS 30

#define TC1_8MHZ_256PS_1SEC_TICKS 31250
#define SECONDS_RESET_VALUE 59

#include <util/delay.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include "light_ws2812/light_ws2812_AVR/Light_WS2812/light_ws2812.h"

/*
/ Modes : SetTime, Counting Down
*/

struct cRGB led[MAX_NUM_LIGHTS];
volatile uint8_t lightIntensity = 50;

volatile uint8_t minutesOnClock = 5;
volatile uint8_t secondsOnClock = SECONDS_RESET_VALUE;
volatile uint8_t timerHasExpired = 0;
volatile uint8_t baseLightIntensity = 50;

void SetOneLightTo(uint8_t lightIndex, uint8_t red, uint8_t green, uint8_t blue) {
    led[lightIndex].r = red;
    led[lightIndex].g = green;
    led[lightIndex].b = blue;
}

void SetLastStringOfLightsTo(uint8_t numberOfLightsToChange, uint8_t red, uint8_t green, uint8_t blue) {
    for(uint8_t i = 0; i < MAX_NUM_LIGHTS; i++) {
        if ( i >= numberOfLightsToChange) {
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
    ws2812_setleds(led, MAX_NUM_LIGHTS);
}

void ClearLights() {
    DDRB |= (1 << PB1);
    PORTB |= (1 << PB1);
    SetLastStringOfLightsTo(MAX_NUM_LIGHTS, 0, 0, 0);
    UpdateLights();
}

/*
    Using 256 prescaler count up to 31,250
*/
void SetupTimer() {
    ICR1 = TC1_8MHZ_256PS_1SEC_TICKS; // TOP Value
    TIMSK1 |= (1 << ICF1); // Enable intterupt when ICR1 value is reached
    TCCR1B |= (1 << WGM13) | (1 << WGM12); // Set to CTC mode
    TCCR1B |= (1 << CS12); // prescaler 256
}

ISR(TIMER1_CAPT_vect) {
    PORTB ^= (1 << PB1);
    if (!timerHasExpired) {
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
    }
}

void ResetTimer() {
    TCNT1 = 1;
    minutesOnClock = 4;
    secondsOnClock = SECONDS_RESET_VALUE;
    timerHasExpired = 0;
}

ISR(INT0_vect) {
    ResetTimer();
}

uint8_t GetPulseOffsetValue() {
    uint8_t timerHighBits = TCNT1H >> 2;
    uint8_t timerLowBits = TCNT1L;

    return timerHighBits;
}

void RunUpdate() {
    uint8_t lightsToLightUp = MAX_NUM_LIGHTS - minutesOnClock - 1;

    uint8_t pulseOffsetValue = GetPulseOffsetValue();

    SetLastStringOfLightsTo(lightsToLightUp, 0, 0, baseLightIntensity);
    SetOneLightTo(lightsToLightUp, 0, baseLightIntensity + pulseOffsetValue, baseLightIntensity);
    UpdateLights();

    _delay_ms(100);
}

void RunTimerCompleteRoutine() {

    for (uint8_t i = MAX_NUM_LIGHTS; i > 0; i--) {
        SetLastStringOfLightsTo(i, 0, baseLightIntensity + i, baseLightIntensity);
        UpdateLights();
        _delay_ms(15);
    }
    for (uint8_t i = 0; i <= MAX_NUM_LIGHTS; i++) {
        SetLastStringOfLightsTo(i, i, baseLightIntensity, baseLightIntensity);
        UpdateLights();
        _delay_ms(15);
    }

}

void SetupInputs() {
    EICRA |= (1 << ISC01) | (1 << ISC00); // INT0 Rising edge
    EIMSK |= (1 << INT0); // enable INT0
}

int main() {

    SetupTimer();
    ClearLights();
    SetupInputs();

    sei();

    while(1) {
        RunUpdate();
        if (timerHasExpired) {
            RunTimerCompleteRoutine();
            timerHasExpired = 0;
        }
    }
}