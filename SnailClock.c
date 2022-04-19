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

struct cRGB led[MAX_NUM_LIGHTS];

volatile uint8_t PCINT21LastState = 0;

volatile struct {
    uint8_t minutes;
    uint8_t menuBrightness;
} timerSettings;

volatile struct {
    uint8_t minutes;
    uint8_t seconds;
} timerState;

struct {
    uint8_t numberOfLightsIlluminated;
    uint8_t isGoingUp;
} expiredState;

volatile enum {INIT, MAIN, COUNTDOWN, EXPIRED} timerMode, nextMode;

volatile uint8_t baseLightIntensity = 50;

void SetOneLightTo(uint8_t lightIndex, uint8_t red, uint8_t green, uint8_t blue) {
    led[lightIndex].r = red;
    led[lightIndex].g = green;
    led[lightIndex].b = blue;
}

void SetLastStringOfLightsTo(uint8_t numberOfLightsToChange, uint8_t red, uint8_t green, uint8_t blue) {
    uint8_t firstLightToChange = MAX_NUM_LIGHTS - numberOfLightsToChange;
    for(uint8_t i = 0; i < MAX_NUM_LIGHTS; i++) {
        if ( i >= firstLightToChange) {
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
    SetLastStringOfLightsTo(MAX_NUM_LIGHTS, 0, 0, 0);
    UpdateLights();
}

//////// Timer Interrupt ////////
ISR(TIMER1_CAPT_vect) {
    if (timerMode == COUNTDOWN) {
        if (timerState.seconds > 0) {
            timerState.seconds--;
        } else {
            if (timerState.minutes > 0) {
                timerState.minutes--;
                timerState.seconds = SECONDS_RESET_VALUE;
            } else {
                nextMode = EXPIRED;
            }
        }
    }
}


//////// Button Interrupts ////////
void SetupInputs() {
    EICRA |= (1 << ISC01) | (1 << ISC00); // INT0 Rising edge
    EIMSK |= (1 << INT0); // enable INT0

    EICRA |= (1 << ISC11) | (1 << ISC10); // INT1 Rising edge
    EIMSK |= (1 << INT1); // enable INT1

    //PCINT21 (PD5) -> START
    PCICR |= (1 << PCIE2);
    PCMSK2 |= (1 << PCINT21);
}

void StartButtonPressed() {
    if (timerMode == MAIN) {
        nextMode = COUNTDOWN;
    } else {
        nextMode = MAIN;
    }
}

void PlusButtonPressed() {
    if (timerMode == MAIN) {
        if (timerSettings.minutes < MAX_NUM_LIGHTS) {
            timerSettings.minutes++;
        }
    }
}

void MinusButtonPressed() {
    if (timerMode == MAIN) {
        if (timerSettings.minutes > 0) {
            timerSettings.minutes--;
        }
    }
}

ISR(INT0_vect) {
    MinusButtonPressed();
}

ISR(INT1_vect) {
    PlusButtonPressed();
}

ISR(PCINT2_vect) {
    if (!PCINT21LastState && (PIND & _BV(PD5))) {
        StartButtonPressed();
    }
    PCINT21LastState = (PIND & _BV(PD5));
}


//////// MAIN MODE UPDATES ////////
void EnterMainMode() {
    timerSettings.menuBrightness = 49;
}

void UpdateMenuBrightness() {
    uint8_t menuBrightnessIsGoingUp = timerSettings.menuBrightness % 2; // Odd is up
    if (menuBrightnessIsGoingUp) {
        timerSettings.menuBrightness += 2;
        if (timerSettings.menuBrightness > 127) {
            timerSettings.menuBrightness = 126;
        }
    } else {
        timerSettings.menuBrightness -= 2;
        if (timerSettings.menuBrightness < 50) {
            timerSettings.menuBrightness = 49;
        }
    }
}

void RunMainModeUpdate() {
    UpdateMenuBrightness();
    SetLastStringOfLightsTo(timerSettings.minutes, 0, timerSettings.menuBrightness, 0);
    UpdateLights();
    _delay_ms(20);
}

//////// Countdown Routine Functions ////////
// Using 256 prescaler count up to 31,250 each second.
void SetupTimer1() {
    ICR1 = TC1_8MHZ_256PS_1SEC_TICKS; // TOP Value
    TIMSK1 |= (1 << ICF1); // Enable intterupt when ICR1 value is reached
    TCCR1B |= (1 << WGM13) | (1 << WGM12); // Set to CTC mode
    TCCR1B |= (1 << CS12); // prescaler 256
}

void StopTimer1() {
    TCCR1B &= ~(_BV(CS12) | _BV(CS11) | _BV(CS10)); // Stop Timer
    TIMSK1 &= ~_BV(ICF1); // Disable intterupt
}

void EnterCountdownMode() {
    TCNT1 = 1;
    SetupTimer1();
    timerState.minutes = timerSettings.minutes;
    timerState.seconds = 0;
}

uint8_t GetPulseOffsetValue() {
    uint8_t timerHighBits = TCNT1H >> 2;
    uint8_t timerLowBits = TCNT1L;

    return timerHighBits;
}

void RunCountdownUpdate() {
    uint8_t lightsToLightUp = timerState.minutes;
    uint8_t indexOfMainLight = (MAX_NUM_LIGHTS - timerState.minutes) - 1;
    uint8_t pulseOffsetValue = GetPulseOffsetValue();

    SetLastStringOfLightsTo(lightsToLightUp, 0, 0, baseLightIntensity);
    SetOneLightTo(indexOfMainLight, 0, baseLightIntensity + pulseOffsetValue, baseLightIntensity);
    UpdateLights();

    _delay_ms(100);
}

void ExitCountdownMode() {
    StopTimer1();
}


//////// Expired Update Functions ////////
void EnterExpiredMode() {
    expiredState.numberOfLightsIlluminated = 0;
    expiredState.isGoingUp = 1;
}

void RunTimerExpiredUpdate() {
    if (expiredState.isGoingUp) {
        expiredState.numberOfLightsIlluminated++;
        if (expiredState.numberOfLightsIlluminated >= MAX_NUM_LIGHTS) {
            expiredState.isGoingUp = 0;
        }
    } else {
        expiredState.numberOfLightsIlluminated--;
        if (expiredState.numberOfLightsIlluminated == 0) {
            expiredState.isGoingUp = 1;
        }
    }

    SetLastStringOfLightsTo(expiredState.numberOfLightsIlluminated, 50, 0, 0);
    UpdateLights();
    _delay_ms(100);
}


////////////////
void CheckForModeChange() {
    if (nextMode != timerMode) {
        cli();
        switch(timerMode) {
            case COUNTDOWN: {
                ExitCountdownMode();
                break;
            }
        }
        switch(nextMode) {
            case INIT: {
                break;
            }
            case MAIN: {
                EnterMainMode();
                break;
            }
            case COUNTDOWN: {
                EnterCountdownMode();
                break;
            }
            case EXPIRED: {
                EnterExpiredMode();
                break;
            }
        }
        timerMode = nextMode;
        sei();
    }
}

int main() {

    timerSettings.minutes = 10;

    timerMode = INIT;

    SetupTimer1();
    ClearLights();
    SetupInputs();

    sei();

    while(1) {
        CheckForModeChange();
        switch(timerMode) {
            case (INIT): {
                nextMode = MAIN;
                break;
            }
            case (MAIN): {
                RunMainModeUpdate();
                break;
            }
            case (COUNTDOWN): {
                RunCountdownUpdate();
                break;
            }
            case (EXPIRED): {
                RunTimerExpiredUpdate();
                break;
            }
        }
    }
}
