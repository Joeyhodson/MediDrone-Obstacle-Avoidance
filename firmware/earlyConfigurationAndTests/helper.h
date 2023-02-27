// Author: Joey Hodson
// 02/22/2023

#include <msp430.h>
#include <msp430fr5738.h>

// General
#define MAX_BYTE            0xFF
#define ON                  0x01
#define OFF                 0x00
// GPIO
#define ECHO                BIT0
#define TRIGGER             BIT1
#define LED                 BIT3
#define LOG_SWITCH          BIT4
#define TOF_ENABLE          BIT0
#define CAN_STBY            BIT3
// Flag(s)
#define RAM_PATCH_FLAG      0x00

/*
 * Pins Taken Directly From the Schematic:
 *
 * P1.0 Echo on HC-SR04 (level shift between)
 * P1.1 Trigger on HC-SR04 (level shift between)
 * P1.2 Interrupt on MCP2517FD-H/SL
 * P1.3 Red Led
 * P1.4 Enable on NXB0104GU12X (w/ external pull-down)
 * P1.5 SPI Clock
 * P1.6 I2C Data
 * P1.7 I2C Clock
 * P2.0 SPI SIMO
 * P2.1 SPI SOMI
 * P2.2 SPI CS
 * PJ.0 Enable on TMF8805-1B
 * PJ.1 extra GPIO
 * PJ.2 extra GPIO
 * PJ.3 Standby on MCP2517FD-H/SL
 * PJ.4 Crystal 1
 * PJ.5 Crystal 1
 */

char isGpioEnabled() {return (!((PM5CTL0 && LOCKLPM5) == 1));}
char areClocksConfigured() {return ((PJSEL0 && BIT4) == 1 || (CSCTL1 && (DCOFSEL0 + DCOFSEL1)) == 1);}

void delay(unsigned long cycles)
{
    for (unsigned int i = 0; i < cycles; i++)
        __delay_cycles(1);
}

void port1SetState(unsigned char state, unsigned char pin)
{
    if (!isGpioEnabled())
        return;
    if (state == ON)
        P1OUT |= pin;
    else
        P1OUT &= ~pin;
}

void portJSetState(unsigned char state, unsigned char pin)
{
    if (!isGpioEnabled())
        return;
    if (state == ON)
        PJOUT |= pin;
    else
        PJOUT &= ~pin;
}

void interruptState(char state) {if(state) __enable_interrupt();else __disable_interrupt();}
void ledState(char state)       {port1SetState(state, LED);}
void triggerState(char state)   {port1SetState(state, TRIGGER);}
void echoState(char state)      {port1SetState(state, ECHO);}
void logSwitchState(char state) {port1SetState(state, LOG_SWITCH);}
void CanStbyState(char state)   {portJSetState(state, CAN_STBY);}
void tofState(char state)       {portJSetState(state, TOF_ENABLE);}

void configureSmclkAndMclk()
{
   // Unlock CS registers
   CSCTL0 = CSKEY;
   // Max setting of DCO (8Mhz)
   CSCTL1 |= DCOFSEL0 + DCOFSEL1;
   // SMCLK = DCO and MCLK = DCO
   CSCTL2 |= SELS_3 + SELM_3;
   // MCLK / 1 = 8Mhz and SMCLK / 8 = 1Mhz
   CSCTL3 |= DIVM_0 + DIVS__8;
   // Re-lock CS registers
   CSCTL0_H = 0;
}

void configureAclk()
{
    // Reroute pins to XIN Crystal
    PJSEL1 &= ~(BIT4);
    PJSEL0 |= (BIT4);
    // Unlock CS registers
    CSCTL0 = CSKEY;
    // ACLK = XT1
    CSCTL2 |= SELA_0;
    // ACLK / 32 = 0.5Mhz
    CSCTL3 |= DIVA__32;
    // Select high frequency mode for XT1 (XTS)
    // Select roper driving power for XT1 (10-16Mhz range from slau272d)
    // Ensure XT1 is on w/o bypass
    CSCTL4 |= (XTS|XT1DRIVE1);
    CSCTL4 &= ~(XT1DRIVE0|XT1BYPASS|XT1OFF);
    // Crystal stabilization loop
    do {
        CSCTL5 &= ~XT1OFFG; // Local fault flag
        SFRIFG1 &= ~OFIFG; // Global fault flag
    } while ((CSCTL5 & XT1OFFG) != 0);
    // Re-lock CS registers
    CSCTL0_H = 0;
}

void configureClocks(char aclk, char smclkAndMclk)
{
    if (aclk == ON) {configureAclk();}
    if (smclkAndMclk == ON) {configureSmclkAndMclk();}
}

void initializeUltrasound()
{
    logSwitchState(ON);
    // Set pin functionality to Timer A
    P1SEL0 |= ECHO;
    P1SEL1 &= ~ECHO;
    // Configure Timer A
    TA0CCTL1 = (CM_3|CCIS_0|SCS|CAP|CCIE);
}

void initializeGPIO()
{
    WDTCTL = (WDTPW|WDTHOLD); // Stop WDT
    PM5CTL0 &= ~LOCKLPM5;     // Enable GPIO pins
    // Port Direction
    P1DIR |= (LED|TRIGGER|LOG_SWITCH); // out
    P1DIR &= ~(ECHO);                  // in
    PJDIR |= (TOF_ENABLE|CAN_STBY);    // out
    P1OUT &= ~(LED|TRIGGER|LOG_SWITCH);
}

void initializeI2C()
{
    UCB0CTL1 |= UCSWRST;
    // Divert pins to I2C functionality
    P1SEL1 |= (BIT6|BIT7);
    P1SEL0 &= ~(BIT6|BIT7);
    // Keep all the default values except the fields below...
    // (UCMode 3:I2C) (Master Mode) (UCSSEL 1:ACLK, 2,3:SMCLK)
    UCB0CTLW0 |= (UCMODE_3 | UCMST | UCSSEL_2 | UCSYNC); // looks fine but ensure aclk is proper frequency
    // Clock divider = 10 (SMCLK @ ~1Mhz / 8 = ~100 Khz)
    UCB0BRW = 10;
    // Exit reset mode
    UCB0CTL1 &= ~UCSWRST;
}
