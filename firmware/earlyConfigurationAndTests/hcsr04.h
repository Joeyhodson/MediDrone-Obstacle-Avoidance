#include <msp430fr5738.h>
#include <msp430.h>
#include "helper.h"

#define STEP_1                          0x00
#define STEP_2                          0x01
#define STEP_3                          0x02
#define MAX_PHASE_ERROR                 0x0A
#define PHASE_SHIFT_DELAY               0x96
#define MINIMUM_TRIGGER_DELAY           0x05
#define PERIOD_IN_MICROSEC              0x0F   // 1/(2^16) -> aclk
#define SPEED_OF_SOUND_FACTOR           0x3A   // 2*(1/speedofSound) (microsec/cm)
// Globals
unsigned char currentCaptureStep, badPhase;
unsigned int fallingEdgeTimestamp, risingEdgeTimestamp, timeDifference;

void configureTimerControl() {TA0CTL = (TASSEL__ACLK|MC__CONTINUOUS|ID_3);}
void clearTimerACounter() {TA0CTL = TACLR;}

void beginRead()
{
    currentCaptureStep = STEP_1;
    triggerState(ON);
    delay(MINIMUM_TRIGGER_DELAY);
    triggerState(OFF);
}

void captureDistanceHelper()
{
    interruptState(OFF);
    configureTimerControl();
    interruptState(ON);
    beginRead();
}

void retryCapture()
{
    clearTimerACounter();
    captureDistanceHelper();
    badPhase = 0;
}

unsigned int calculateDistance()
{
    timeDifference = fallingEdgeTimestamp - risingEdgeTimestamp;
    return (PERIOD_IN_MICROSEC * timeDifference) / SPEED_OF_SOUND_FACTOR;
}

unsigned int captureDistance()
{
    captureDistanceHelper();
    while(currentCaptureStep != STEP_3)
    {
        if(badPhase == MAX_PHASE_ERROR)
            retryCapture();
        // 'randomly' shifts phase until echo (timer A) follows directly after trigger
        delay(PHASE_SHIFT_DELAY);
        badPhase++;
    }
    interruptState(OFF);
    clearTimerACounter();
    return calculateDistance();
}

// Timer0_A1 CC1-2, Interrupt Handler
#pragma vector = TIMER0_A1_VECTOR
__interrupt void Timer0_A1_ISR(void)
{
    if (__even_in_range(TA0IV, TA0IV_TAIFG) == TA0IV_TA0CCR1)
        if(currentCaptureStep == STEP_1)
        {
            // Capture the timer value during echo's rising edge (ranging start)
            risingEdgeTimestamp = TA0CCR1;
            currentCaptureStep = STEP_2;
        }
        else if(currentCaptureStep == STEP_2)
        {
            // Capture the timer value during echo's falling edge (ranging stops)
            fallingEdgeTimestamp = TA0CCR1;
            currentCaptureStep = STEP_3;
        }
}

