#include "tmf8805.h"
#include "mcp2517.h"
#include <msp430.h>
#include <msp430fr5738.h>

//#include "./gitExample/tof.c"
unsigned char* readDistanceFromTof();
unsigned int readDistanceFromUltrasound();
unsigned char downloadRamPatch();
unsigned char initializeTof();
unsigned char startTof();
unsigned char resetTof();
unsigned char selfTest();

/*
 * | BOARD # | PASS / FAIL |       TEST      |
 * |   2     |   PASS      |  init tof comm  |
 * |   3     |   PASS      |  init tof comm  |
 * |   5     |   PASS      |  init tof comm  |
 * |   4     |   PASS      |  init tof comm  |
 * |   1     |   FAIL      |  init tof comm  |
 */


unsigned int readDistanceFromUltrasound() {return captureDistance();}

unsigned char* readDistanceFromTimeOfFlight()
{
    unsigned char results[READ_ELEVEN_BYTE] = {MAX_BYTE};
    performWriteSequence(START_APP0_KEY);
    performReadSequence(READ_RESULTS_KEY, results);
    return results;
}

unsigned char downloadRamPatch()
{
    if (performWriteSequence(RAM_PATCH_KEY))
    {
        delay(100);
        unsigned char currentApp[READ_ONE_BYTE] = {MAX_BYTE};
        delay(10);
        while(currentApp[0] != BOOTLOADER && currentApp[0] != APP_ZERO && currentApp[0])
        {
            performWriteSequence(WAKEUP_FROM_STANDBY_KEY);
            delay(50);
            performReadSequence(IS_CPU_READY_KEY, currentApp);
            delay(50);
            performReadSequence(DISCOVER_RUNNING_APP_KEY, currentApp);
            delay(50);
        }
        performReadSequence(DISCOVER_RUNNING_APP_KEY, currentApp);
    }
    return 0;
}

unsigned char resetTof()
{
    tofState(ON);
    return performWriteSequence(RAM_RESET_KEY);
}

unsigned char startTof()
{
    const unsigned char cpuIsReady = TMF8805_ADDRESS;
    unsigned char tofStatusArr[READ_ONE_BYTE] = {MAX_BYTE};

    tofState(ON);
    delay(30000);

    // user timer A interrupt to start a timeout here
    while(tofStatusArr[0] != cpuIsReady)
    {
        delay(50);
        performWriteSequence(WAKEUP_FROM_STANDBY_KEY);
        delay(50);
        performReadSequence(IS_CPU_READY_KEY, tofStatusArr);
    }
    return 1;
}

unsigned char initializeTof()
{
    unsigned char currentApp[READ_ONE_BYTE] = {MAX_BYTE};
    while(currentApp[0] != BOOTLOADER && currentApp[0] != APP_ZERO)
    {
        delay(10);
        performReadSequence(DISCOVER_RUNNING_APP_KEY, currentApp);
    }

    int tofInitialized = 0;
    switch((currentApp[0])) {
        case BOOTLOADER:
            if (RAM_PATCH_FLAG)
                tofInitialized = downloadRamPatch();
            break;
        case APP_ZERO:
            tofInitialized = 1;
            break;
        default:
            tofInitialized = 0;
    }
    return tofInitialized;
}

unsigned char selfTest()
{
    // more logic here
    return 1;
}

int main(void)
{
    initializeGPIO();
    if(!selfTest())
        return OFF;

    configureClocks(ON, ON);
    canStbyState(OFF);

    initializeUART();
    initializeUltrasound();
    while(1)
    {
        //uartWriteByte(0x55);
        if (readDistanceFromUltrasound() < 100)
            ledState(ON);
        else
            ledState(OFF);
    }

    /*
     *
    //initializeI2C();
    //DRV_SPI_Initialize();
    //if (startTof())
        //initializeTof();
    basicCANConfiguration();
    delay(10000);
    initializeRAMAndSelectNormalMode();
    delay(10000);
    char i = 1;
    while(i != 50)
    {
        i++;
        transmitMessageFromTXFIFO();
        delay(1000);
    }
    readMessageFromTEF();
    //filterConfigurationToMatchAStandardFrameRange();
    //while(1)
        //receiveCANMessage();
    //configureTBC();
    //_low_power_mode_0();
     */
}
