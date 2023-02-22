#include <msp430.h>
#include <msp430fr5738.h>
#include "tmf8805.h"

// General Constants
#define MAX_BYTE 0xFF
#define redLED BIT3 // Red Led at P1.3
#define ON 1
#define OFF 0

// TMF8805
#define TMF8805_ADDRESS 0x41
#define ENABLE_REGISTER 0xE0
#define INITIALIZATION_KEY 0x01


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

/*
 * Recommended setup directly from slau272d:
 * 1) Set UCSWRST
 * 2) Initialize all eUSCI_B registers with UCSWRST = 1 (including UCxCTL1)
 * 3) Configure Ports
 * 4) Clear UCSWRST through software
 * 5) Enable Interrupts (optional)
 *
 *
 *
 * Notes:
 *
 * eUSCI_B -> supports the 7-bit addressing required by TMF8805 Registers
 *
 * UCB0CTLW0 (word 0) and UCB0CTLW1 (word 1) present in 5738
 * UCSWRST present in 5738 header w/ comment "USCI Software Reset"
 *
 * P1.6 I2C Data
 * P1.7 I2C Clock
 *
 * From slau272d : "Port function selection. Each bit corresponds to one channel on Port x.
 * The values of each bit position in PxSEL1 and PxSEL0 are combined to specify
 * the function. For example, if P1SEL1.5 = 1 and P1SEL0.5 = 0, then the
 * secondary module function is selected for P1.5.
 * See PxSEL1 for the definition of each value."
 *
 * e.g.
 * PxSEL1 -- PXSEL0 -- Function
 *   0    --   0    --   GPIO
 *   0    --   1    --   Primary Module
 *   1    --   0    --   Secondary Module
 *   1    --   1    --   Tertiary Module
 *
 * in 6989, P4.0 is I2C data on USCI_B1
 * in 6989, P4.1 is I2c clock on USCI_B1
 *
 * From TMF8805:
 * if Vdd > 3.0v (which it is)
 * then < 1Mhz I2C Speed. (16mHz / 32 = 0.5kHz)
 * route smclk to the crystal?
 *
 * VLO of 5789 is 8.3 kHz (might be suitable)
 *
 *
 */

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

void ledState(int state)
{
    if (!isGpioEnabled())
        return;
    P1OUT &= ~redLED;
    if (state == ON)
        P1OUT |= redLED;
}

void initializeGPIO()
{
    WDTCTL = WDTPW | WDTHOLD; // Stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5; // Enable the GPIO pins

    // Port direction
    P1DIR |= redLED; // output
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

void delay(int seconds)
{
    if (!clocksAreConfigured())
        return;

    // Through empirical analysis, in up-down mode (up to max TA0CCR0 [2^16] and back down),
    // it took us 0.132sec. Therefore, 0.132sec / (2*65535) gives us 1Mhz as expected
    // since SMCLK is 8Mhz / 8 = 1Mhz

    // Configure capture register with max value / 10 for higher precision later on
    TA0CCR0 = 6554;
    // Timer_A: SMCLK, divide by 8, up-down mode, clear TAR (leaves TAIE=0)
    TA0CTL = TASSEL_2 | ID_3 | MC_3 | TACLR;

    // Clear flag
    TA0CTL &= ~TAIFG;

    for (int i = seconds; i >= 1; i--)
    {
        // 75 iterations of TA0CCR0 = 6554 gives us roughly 1sec at 1Mhz
        for (int i = 75; i >= 1; i--)
        {
            // Flag is set once TAR reaches TA0CCR0 and then back down to 0
            while ((TA0CTL & TAIFG) != TAIFG) {}
            TA0CTL &= ~TAIFG;
        }
    }

    // Turn off Timer A
    TA0CTL |= MC_0;
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

void configureClocks(int aclk, int smclkAndMclk)
{
    if (aclk == ON)
        configureAclk();
    if (smclkAndMclk == ON)
        configureSmclkAndMclk();
}

// Write a word (2 bytes) to I2C (address, register)
int i2cWriteWordToRegister(unsigned char i2cAddress, unsigned char i2cRegister, unsigned int data)
{
    unsigned char byte1, byte2;
    // MSB
    byte1 = (data >> 8) & 0xFF;
    // LSB
    byte2 = data & 0xFF;

    // Set I2C address
    UCB0I2CSA = i2cAddress;
    // Master writes (R/W bit = Write)
    UCB0CTLW0 |= UCTR;
    // Clear transmit flag
    UCB0IFG &= ~UCTXIFG0;

    // Initiate the Start Signal
    UCB0CTLW0 |= UCTXSTT;
    // Ensures slave address is cleared from buffer
    while ((UCB0IFG & UCTXIFG0) == 0) {}
    UCB0TXBUF = i2cRegister;
    while ((UCB0CTLW0 & UCTXSTT) != 0) {}
    // added this now works to write register
    // (ensures register is cleared from buffer)
    while ((UCB0IFG & UCTXIFG0) == 0) {}

    // Write byte 1
    UCB0TXBUF = byte1;
    while ((UCB0IFG & UCTXIFG0) == 0) {}

    // Write byte 2
    UCB0TXBUF = byte2;
    while ((UCB0IFG & UCTXIFG0) == 0) {}

    // Setup the stop signal
    UCB0CTLW0 |= UCTXSTP;
    while ((UCB0CTLW0 & UCTXSTP) != 0) {}

    return 1;
}

int i2cWriteByteToRegister(unsigned char i2cAddress, unsigned char i2cRegister, unsigned char data)
{
    // Set I2C address
    UCB0I2CSA = i2cAddress;
    // Master writes (R/W bit = Write)
    UCB0CTLW0 |= UCTR;
    // Clear transmit flag
    UCB0IFG &= ~UCTXIFG0;
    // Initiate the Start Signal
    UCB0CTLW0 |= UCTXSTT;
    while ((UCB0IFG & UCTXIFG0) == 0) {}
    // Byte = register address
    UCB0TXBUF = i2cRegister;
    while ((UCB0CTLW0 & UCTXSTT) != 0) {}
    // added this now works to write register
    while ((UCB0IFG & UCTXIFG0) == 0) {}
    // Write byte 1
    UCB0TXBUF = data;
    while ((UCB0IFG & UCTXIFG0) == 0) {}

    // Setup the stop signal
    UCB0CTLW0 |= UCTXSTP;
    while ((UCB0CTLW0 & UCTXSTP) != 0) {}

    return 1;
}

// Read a word (2 bytes) from I2C (address, register)
int i2cReadWordFromRegister(unsigned char i2cAddress, unsigned char i2cRegister, unsigned int * data)
{
    unsigned char byte1, byte2;

    // Initialize bytes
    byte1 = 111;
    byte2 = 111;

    // Write frame 1
    // Set I2C address
    UCB0I2CSA = i2cAddress;
    // Clear transmit flag
    UCB0IFG &= ~UCTXIFG0;

    // Master writes (R/W bit = Write)
    UCB0CTLW0 |= UCTR;
    // Initiate the start signal
    UCB0CTLW0 |= UCTXSTT;
    while ((UCB0IFG & UCTXIFG0) == 0) {}
    // Byte = register address
    UCB0TXBUF = i2cRegister;
    while ((UCB0IFG & UCTXIFG0) == 0) {}
    while ((UCB0CTLW0 & UCTXSTT) != 0) {}
    if ((UCB0IFG & UCNACKIFG) != 0)
        return 0;
    // Master reads (R/W bit = Read)
    UCB0CTLW0 &= ~UCTR;
    // Initiate a repeated start signal
    UCB0CTLW0 |= UCTXSTT;

    //Read frame 1
    while ((UCB0IFG & UCRXIFG0) == 0) {}
    byte1 = UCB0RXBUF;

    // Read frame 2
    while ((UCB0CTLW0 & UCTXSTT) != 0) {}
    // Setup the stop signal
    UCB0CTLW0 |= UCTXSTP;
    while ((UCB0IFG & UCRXIFG0) == 0) {}
    byte2 = UCB0RXBUF;
    while ((UCB0CTLW0 & UCTXSTP) != 0) {}

    // Merge the two received bytes
    *data = ((byte1 << 8) | (byte2 & 0xFF));

    return 1;
}

// Read a word (2 bytes) from I2C (address, register)
int i2cReadByteFromRegister(unsigned char i2cAddress, unsigned char i2cRegister, unsigned char * data)
{
    unsigned char byte;

    // Initialize the byte to make sure data is received every time
    byte = 111;

    // Write frame 1
    // Set I2C address
    UCB0I2CSA = i2cAddress;
    // Clear transmit flag
    UCB0IFG &= ~UCTXIFG0;

    // Master writes (R/W bit = Write)
    UCB0CTLW0 |= UCTR;
    // Initiate the start signal
    UCB0CTLW0 |= UCTXSTT;
    while ((UCB0IFG & UCTXIFG0) == 0) {}
    // Byte = register address
    UCB0TXBUF = i2cRegister;
    while ((UCB0IFG & UCTXIFG0) == 0) {}
    while ((UCB0CTLW0 & UCTXSTT) != 0) {}
    if ((UCB0IFG & UCNACKIFG) != 0)
        return 0;
    // Master reads (R/W bit = Read)
    UCB0CTLW0 &= ~UCTR;
    // Initiate a repeated start signal
    UCB0CTLW0 |= UCTXSTT;
    while ((UCB0CTLW0 & UCTXSTT) != 0) {}
    //Read frame 1
    while ((UCB0IFG & UCRXIFG0) == 0) {}
    byte = UCB0RXBUF;

    //while ((UCB0CTLW0 & UCTXSTT) != 0) {}
    // Setup the stop signal
    UCB0CTLW0 |= UCTXSTP;
    while ((UCB0CTLW0 & UCTXSTP) != 0) {}

    *data = byte;

    return 1;
}

void tofState(int state)
{
    PJDIR |= BIT0;
    PJOUT &= ~BIT0;
    if (state == ON)
        PJOUT |= BIT0;
}

int checkTof()
{
    unsigned char tofStatus = MAX_BYTE;
    const unsigned char cpuIsReady = TMF8805_ADDRESS;

    tofState(ON);
    _delay_cycles(30000);

    // user timer A interrupt to start a timeout here
    while(tofStatus != cpuIsReady)
    {
        _delay_cycles(15000);
        i2cWriteByteToRegister(TMF8805_ADDRESS, ENABLE_REGISTER, INITIALIZATION_KEY);
        _delay_cycles(500);
        i2cReadByteFromRegister(TMF8805_ADDRESS, ENABLE_REGISTER, &tofStatus);
    }

    return 1;
}

int checkTofTwo()
{
    unsigned char tofStatus = MAX_BYTE;
    const unsigned char cpuIsReady = TMF8805_ADDRESS;

    //unsigned char tofStatusArr[14] = {MAX_BYTE};

    tofState(ON);
    _delay_cycles(30000);

    // user timer A interrupt to start a timeout here
    while(tofStatus != cpuIsReady)
    {
        _delay_cycles(15000);
        performWriteSequence(WAKEUP_FROM_STANDBY_KEY);
        //i2cWriteByteToRegister(TMF8805_ADDRESS, ENABLE_REGISTER, INITIALIZATION_KEY);
        _delay_cycles(500);
        performReadSequence(IS_CPU_READY_KEY, &tofStatus);
        //i2cReadByteFromRegister(TMF8805_ADDRESS, ENABLE_REGISTER, &tofStatus);
    }

    return 1;
}

/*
int initializeTof()
{
    // pull enable line high
    // write a 0x1 to register 0xE0
    // poll register 0xE0 until 0x41 is read back
    // read register 0x00 (AppID) to see what app is running
    // if bootloader (reg 0x00 = 0x80) download ram patch (i.e. return -1)
        // Start ram patch
    // else if App0 is running 0xC0 from 0x00
        // talk to App0

    int tofInitialized = 0;
    checkTof();

    // more can be done here
    unsigned char appVersion = MAX_BYTE;
    _delay_cycles(500);
    while(appVersion != MAX_BYTE)
    {
        i2cReadByteFromRegister(TMF8805_ADDRESS, REGISTER_ZERO, &appVersion);
    }

    switch(appVersion)
    {
        case BOOTLOADER:
            if (RAM_PATCH_FLAG)
                downloadRamPatch(discoverRunningApp());
            break;
        case APP_ZERO:
            tofInitialized = 1;
            break;
        default:
            break;
    }

    return tofInitialized;
}
*/
/*
 * | BOARD # | PASS / FAIL |       TEST      |
 * |   2     |   PASS      |  init tof comm  |
 * |   3     |   PASS      |  init tof comm  |
 * |   5     |   PASS      |  init tof comm  |
 * |   4     |   PASS      |  init tof comm  |
 * |   1     |   FAIL      |  init tof comm  |
 */

int main(void) {

    initializeGPIO();
    configureClocks(OFF, ON);
    initializeI2C();
    if (checkTofTwo())
        ledState(ON);
    //if (initializeTof())
        //computeDistance(runAppZero());
    //runApp0FromTof();


    //_low_power_mode_0();
}
