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
#define CHIP_SELECT         BIT2
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
void canStbyState(char state)   {portJSetState(state, CAN_STBY);}
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

void configureAclkWithCrystal()
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
        ledState(ON);
        delay(5000);
        ledState(OFF);
        delay(5000);
    } while ((CSCTL5 & XT1OFFG) != 0);
    // Re-lock CS registers
    CSCTL0_H = 0;
}

void configureAclkWithDCO()
{
    // Unlock CS registers
    CSCTL0 = CSKEY;
    // ACLK = XT1
    CSCTL2 |= SELA_3;
    // ACLK / 1 = 10hz
    CSCTL3 |= DIVA__32;
    // Re-lock CS registers
    CSCTL0_H = 0;
}

void configureClocks(char aclk, char smclkAndMclk)
{
    if (aclk == ON) {configureAclkWithDCO();}
    if (smclkAndMclk == ON) {configureSmclkAndMclk();}
}

void configureTimerControl() {TA0CTL = (TASSEL__ACLK|MC__CONTINUOUS|ID_2);}

void initializeUltrasound()
{
    logSwitchState(ON);
    configureTimerControl();
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
    PJOUT &= ~(TOF_ENABLE|CAN_STBY);
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

// Configure UART to the popular configuration
// 9600 baud, 8-bit data, LSB first, no parity bits, 1 stop bit
// no flow control
// Initial clock: SMCLK @ 1.048 MHz with oversampling
void initializeUART(void)
{
    UCA0CTLW0 |= UCSWRST;
    // Divert pins to UART functionality
    P2SEL1 &= ~(BIT0|BIT1);
    P2SEL0 |= (BIT0|BIT1);
    //P2DIR |= (BIT0);
    // Use SMCLK clock; leave other settings default
    UCA0CTLW0 |= (UCMODE0|UCSSEL__SMCLK);
    UCA0CTLW0 &= ~(UCSYNC|UCPEN|UC7BIT);
    // Configure the clock dividers and modulators
    // UCBR=6, UCBRF=13, UCBRS=0x22, UCOS16=1 (oversampling)
    UCA0BRW = 8;
    UCA0MCTLW = (UCBRS5|UCBRS6|UCBRS7);
    // listen mode
    UCA0STATW |= UCLISTEN;
    // Exit the reset state (so transmission/reception can begin)
    UCA0CTLW0 &= ~UCSWRST;
}

void uartWriteByte(unsigned char byte)
{
    // Wait for any ongoing transmission to complete
    while ((UCA0IFG & UCTXIFG) == 0) {}
    // Write the byte to the transmit buffer
    UCA0TXBUF = byte;
}

// The function returns the byte; if none received, returns NULL
unsigned char uartReadChar(void)
{
    unsigned char byte;
    // Return NULL if no byte received
    while ((UCA0IFG & UCRXIFG) == 0) {}
    byte = UCA0RXBUF;
    return byte;
}

/*
 * SPI Initialization & transfer functions found under MCP25xx API
 */
