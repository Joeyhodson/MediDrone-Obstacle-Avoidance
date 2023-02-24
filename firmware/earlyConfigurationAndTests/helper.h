/*
 * Helper library for MediDrone-Obstacle-Avoidance
 * 02/22/2023
 * Joey Hodson
 */

#include <msp430.h>
#include <msp430fr5738.h>

/*
 * Pin-outs Taken Directly From the Schematic:
 *
 * P1.0 Echo on HC-SR04 (level shift between)
 * P1.1 Trigger on HC-SR04 (level shift between)
 * P1.2 Interrupt on MCP2517FD-H/SL
 * P1.3 Red Led
 * P1.4 Enable on NXB0104GU12X (w/ external pull-down)
 * P1.5 SPI Clock
 * P1.6 I2C Data
 * P1.7 I2C Clock
 * P2.0 SPI SIMO (slave in master out)
 * P2.1 SPI SOMI (slave out master in)
 * P2.2 SPI CS (chip select)
 * PJ.0 Enable on TMF8805-1B
 * PJ.1 extra GPIO
 * PJ.2 extra GPIO
 * PJ.3 Standby on MCP2517FD-H/SL
 * PJ.4 Crystal 1
 * PJ.5 Crystal 1
 */

// General
#define ON 1
#define OFF 0
#define LED BIT3

int isGpioEnabled()
{
    int state = 1;
    if ((PM5CTL0 && LOCKLPM5) == 1)
        state = 0;
    return state;
}

int clocksAreConfigured()
{
    int config = 0;
    if ((PJSEL0 && BIT4) == 1 || (CSCTL1 && (DCOFSEL0 + DCOFSEL1)) == 1)
        config = 1;
    return config;
}

void delay(unsigned long cycles)
{
    for (unsigned int i = 0; i < cycles; i++)
        __delay_cycles(1);
}

void tofState(int state)
{
    PJDIR |= BIT0;
    PJOUT &= ~BIT0;
    if (state == ON)
        PJOUT |= BIT0;
}

void configureSmclkAndMclk()
{
   // Unlock CS registers
   CSCTL0 = CSKEY;
   // Max setting of DCO (8Mhz)
   CSCTL1 |= DCOFSEL0 + DCOFSEL1;
   // SMCLK = DCO and MCLK = DCO
   CSCTL2 |= SELS_3 + SELM_3;
   // MCLK / 1 = 8Mhz and SMCLK / 8 = 1Mhz
   CSCTL3 |= DIVM_0 + DIVS_3;
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
    // ACLK / 1 = 16Mhz
    CSCTL3 |= DIVA_0;

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

void configureClocks(int aclk, int smclkAndMclk)
{
    if (aclk == ON)
        configureAclk();
    if (smclkAndMclk == ON)
        configureSmclkAndMclk();
}

void ledState(int state)
{
    if (!isGpioEnabled())
        return;
    P1OUT &= ~LED;
    if (state == ON)
        P1OUT |= LED;
}

void initializeGPIO()
{
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5; // Enable the GPIO pins

    // Port direction
    P1DIR |= LED; // output
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
