#include <msp430.h>
#include <msp430fr5738.h>

#define redLED BIT3 // Red Led at P1.3
#define ON 1
#define OFF 0

#define FLAGS UCA1IFG // Contains the transmit & receive flags
#define RXFLAG UCRXIFG // Receive flag
#define TXFLAG UCTXIFG // Transmit flag
#define TXBUFFER UCA1TXBUF // Transmit buffer
#define RXBUFFER UCA1RXBUF // Receive buffer

// TMF8805
#define TMF8805_ADDRESS 0x41
#define ENABLE_REGISTER 0xE0
//const byte foo = 0x1;

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

// reference for launchpad code
/* void initialize_i2c_launchPad(void)
{
    UCB1CTLW0 |= UCSWRST;

    // Divert pins to I2C functionality
    P4SEL1 |= (BIT1|BIT0);
    P4SEL0 &= ~(BIT1|BIT0);

    // Keep all the default values except the fields below...
    // (UCMode 3:I2C) (Master Mode) (UCSSEL 1:ACLK, 2,3:SMCLK)
    UCB1CTLW0 |= UCMODE_3 | UCMST | UCSSEL_3;

    // Clock divider = 8 (SMCLK @ 1.048 MHz / 8 = 131 KHz)
    UCB1BRW = 8;

    // Exit the reset mode
    UCB1CTLW0 &= ~UCSWRST;
} */

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

void led(int state)
{
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
    // try UCB0CTLW0 |= UCSWRST;

    // Divert pins to I2C functionality
    P1SEL1 |= (BIT6|BIT7);
    P1SEL0 &= ~(BIT6|BIT7);

    // Keep all the default values except the fields below...
    // (UCMode 3:I2C) (Master Mode) (UCSSEL 1:ACLK, 2,3:SMCLK)
    UCB0CTLW0 |= UCMODE_3 | UCMST | UCSSEL_3; // looks fine but ensure aclk is proper frequency

    // Clock divider = 32 (ACLK @ 16.048 MHz / 32 = ~500 KHz)
    UCB0BRW = 32;

    // Exit the reset mode
    UCB0CTL1 &= ~UCSWRST;
    // try UCB0CTLW0 &= ~UCSWRST;
    // slau272d had UCB0CTL1 &= ^UCSWRST;
}

int clocksAreConfigured()
{
    int config = 0;
    if ((PJSEL0 && BIT4) == 1)
        config = 1;
    return config;
}

void configureClocks()
{
    // Reroute pins to XIN Crystal
    PJSEL1 &= ~(BIT4);
    PJSEL0 |= (BIT4);

    // Unlock CS registers
    CSCTL0 = CSKEY;
    // Max setting of DCO (8Mhz)
    //CSCTL1 |= DCOFSEL0 + DCOFSEL1;
    __delay_cycles(10000);
    CSCTL1 &= ~(DCOFSEL0 | DCOFSEL1);
    CSCTL1 &= ~(DCORSEL);
    // ACLK = XT1 + SMCLK = DCO + MCLK = DCO
    CSCTL2 = SELA_0 + SELS_3 + SELM_3;
    // ACLK / 1 = 16Mhz + MCLK / 1 = 8Mhz + SMCLK / 1 = 8Mhz
    CSCTL3 = DIVA_0 + DIVS_0 + DIVM_0;

    // Select high frequency mode for XT1 (XTS)
    // Select roper driving power for XT1 (10-16Mhz range from slau272d)
    // Ensure XT1 is on w/o bypass

    CSCTL4 |= (XTS|XT1DRIVE1);
    CSCTL4 &= ~(XT1DRIVE0|XT1BYPASS|XT1OFF);

    // More drive power to oscillator (16-24Mhz) if needed
    //CSCTL4 |= (XTS|XT1DRIVE1|XT1DRIVE0);
    //CSCTL4 &= ~(XT1BYPASS|XT1OFF);

    // Crystal stabilization loop
    do {
        CSCTL5 &= ~XT1OFFG; // Local fault flag
        SFRIFG1 &= ~OFIFG; // Global fault flag
        led(ON);
    } while ((CSCTL5 & XT1OFFG) != 0);
    led(OFF);
    // Re-lock CS registers
    CSCTL0_H = 0;
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
    TA0CTL = TASSEL_1 | ID_3 | MC_3 | TACLR;

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

// Write a word (2 bytes) to I2C (address, register)
int i2cWriteWord(unsigned char i2cAddress, unsigned char i2cRegister, unsigned int data)
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
    // Initiate the Start Signal
    UCB0CTLW0 |= UCTXSTT;
    while ((UCB0IFG & UCTXIFG0) == 0) {}
    // Byte = register address
    UCB0TXBUF = i2cRegister;
    while ((UCB0CTLW0 & UCTXSTT) != 0) {}

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

// Write a byte to I2C (address, register)
int i2cWriteByte(unsigned char i2cAddress, unsigned char i2cRegister, unsigned char data)
{
    // Set I2C address
    UCB0I2CSA = i2cAddress;

    // Master writes (R/W bit = Write)
    UCB0CTLW0 |= UCTR;

    // Initiate the Start Signal
    UCB0CTLW0 |= UCTXSTT;
    while ((UCB0IFG & UCTXIFG0) == 0) {}

    // Byte = register address
    UCB0TXBUF = i2cRegister;
    while ((UCB0CTLW0 & UCTXSTT) != 0) {}

    // Write byte
    UCB0TXBUF = data;
    while ((UCB0IFG & UCTXIFG0) == 0) {}

    // Setup the stop signal
    UCB0CTLW0 |= UCTXSTP;
    while ((UCB0CTLW0 & UCTXSTP) != 0) {}

    return 1;
}

// Read a word (2 bytes) from I2C (address, register)
int i2cReadWord(unsigned char i2cAddress, unsigned char i2cRegister, unsigned int * data)
{
    unsigned char byte1, byte2;

    // Initialize the bytes to make sure data is received every time
    byte1 = 111;
    byte2 = 111;

    // Write frame 1
    // Set I2C address
    UCB0I2CSA = i2cAddress;
    UCB0IFG &= ~UCTXIFG0;
    // Master writes (R/W bit = Write)
    UCB0CTLW0 |= UCTR;
    // Initiate the start signal
    UCB0CTLW0 |= UCTXSTT;
    while ((UCB0IFG & UCTXIFG0) == 0) {}
    // Byte = register address
    UCB0TXBUF = i2cRegister;
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

void initializeToF()
{
    // pull enable line high
    // write a 0x1 to register 0xE0
    // poll register 0xE0 until 0x41 is read back
    // read register 0x00 (AppID) to see what app is running
    // if bootloader (reg 0x00 = 0x80) download ram patch (i.e. return -1)
        // Start ram patch
    // else if App0 is running 0xC0 from 0x00
        // talk to App0

    unsigned int data = 0xFFFF;

    // pull enable line high
    PJDIR |= BIT0;
    PJOUT |= BIT0;

    // write a 0x1 to register 0xE0
    if (i2cWriteByte(TMF8805_ADDRESS, ENABLE_REGISTER, 0x01))
    {
        //i2cReadWord(TMF8805_ADDRESS, ENABLE_REGISTER, &data);
        led(ON);
    }

    if (data == ENABLE_REGISTER)
        return; // return 1

}

int main(void)
{
    // Debug
    int checkGPIO = 1;
    int checkClock = 1;
    int checkI2C = 1;
    int checkToF = 1;
    // ...

    if (checkGPIO)
        initializeGPIO();
    if (checkGPIO && checkClock)
        configureClocks();
    if (checkGPIO && checkI2C)
        initializeI2C();
    __delay_cycles(10000);
    if (checkGPIO && checkI2C && checkToF)
        initializeToF();

    led(ON);
    delay(5);
    led(OFF);


    //led(ON);
    //delay(5);
    //led(OFF);

    //_low_power_mode_0();
}
