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

volatile uint8_t PCINT21LastState = 0;

volatile struct {
    uint8_t minutes;
    uint8_t menuBrightness;
} timerSettings;

volatile struct {
    uint8_t minutes;
    uint8_t seconds;
    uint8_t isRunning;
} timerState;

enum {INIT, MAIN, COUNTDOWN, EXPIRED} timerMode;

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
    DDRB |= (1 << PB1);
    PORTB |= (1 << PB1);
    SetLastStringOfLightsTo(MAX_NUM_LIGHTS, 0, 0, 0);
    UpdateLights();
}

/*
    Using 256 prescaler count up to 31,250
*/
void SetupTimer1() {
    ICR1 = TC1_8MHZ_256PS_1SEC_TICKS; // TOP Value
    TIMSK1 |= (1 << ICF1); // Enable intterupt when ICR1 value is reached
    TCCR1B |= (1 << WGM13) | (1 << WGM12); // Set to CTC mode
    TCCR1B |= (1 << CS12); // prescaler 256
}

ISR(TIMER1_CAPT_vect) {
    PORTB ^= (1 << PB1);
    if (timerState.isRunning) {
        if (timerState.seconds > 0) {
            timerState.seconds--;
        } else {
            if (timerState.minutes > 0) {
                timerState.minutes--;
                timerState.seconds = SECONDS_RESET_VALUE;
            } else {
                timerState.isRunning = 0;
            }
        }
    }
}


//////// Button Interrupts
void ResetCountdownTimer() {
    TCNT1 = 1;
    timerState.minutes = timerSettings.minutes;
    timerState.seconds = SECONDS_RESET_VALUE;
    timerState.isRunning = 1;
}

void StartButtonPressed() {
    if (timerMode == MAIN) {
        timerMode = COUNTDOWN;
        ResetCountdownTimer();
    } else {
        timerMode = MAIN;
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

// Setup button PD5/PCINT21
ISR(PCINT2_vect) {
    if (!PCINT21LastState && (PIND & _BV(PD5))) {
        StartButtonPressed();
    }
    PCINT21LastState = (PIND & PD5);
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

// void TestSettingLastLights() {

//     for (uint8_t i = 0; i <= MAX_NUM_LIGHTS; i++) {
//         SetLastStringOfLightsTo(i, 0, 0, 127);
//         UpdateLights();
//         _delay_ms(500);
//     }
// }

// void TestSettingOneLight() {
//     for (uint8_t i = 0; i < MAX_NUM_LIGHTS; i++) {
//         ClearLights();
//         SetOneLightTo(i, 0, 0, 127);
//         UpdateLights();
//         _delay_ms(500);
//     }
// }

//////// Initialization Functions ////////
void SetupInputs() {
    EICRA |= (1 << ISC01) | (1 << ISC00); // INT0 Rising edge
    EIMSK |= (1 << INT0); // enable INT0

    EICRA |= (1 << ISC11) | (1 << ISC10); // INT1 Rising edge
    EIMSK |= (1 << INT1); // enable INT1

    //PCINT21 (PD5) -> START
    PCICR |= (1 << PCIE2);
    PCMSK2 |= (1 << PCINT21);
}

//////// MAIN MODE UPDATES ////////
void UpdateMenuBrightness() {
    uint8_t menuBrightnessIsGoingUp = timerSettings.menuBrightness % 2; // Odd is up
    if (menuBrightnessIsGoingUp) {
        timerSettings.menuBrightness += 2;
        if (timerSettings.menuBrightness > 200) {
            timerSettings.menuBrightness = 200;
        }
    } else {
        timerSettings.menuBrightness -= 2;
        if (timerSettings.menuBrightness < 100) {
            timerSettings.menuBrightness = 99;
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
uint8_t GetPulseOffsetValue() {
    uint8_t timerHighBits = TCNT1H >> 2;
    uint8_t timerLowBits = TCNT1L;

    return timerHighBits;
}

void RunCountdownUpdate() {
    uint8_t lightsToLightUp = timerState.minutes;
    uint8_t indexOfMainLight = MAX_NUM_LIGHTS - timerState.minutes;
    uint8_t pulseOffsetValue = GetPulseOffsetValue();

    SetLastStringOfLightsTo(lightsToLightUp, 0, 0, baseLightIntensity);
    SetOneLightTo(indexOfMainLight, 0, baseLightIntensity + pulseOffsetValue, baseLightIntensity);
    UpdateLights();

    _delay_ms(100);
}

int main() {

    timerSettings.minutes = 10;

    timerMode = INIT;
    timerState.minutes = 5;
    timerState.seconds = 0;

    SetupTimer1();
    ClearLights();
    SetupInputs();

    sei();

    while(1) {
        switch(timerMode) {
            case (INIT): {
                timerMode = MAIN;
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
                 break;
            }
        }
    }
}
