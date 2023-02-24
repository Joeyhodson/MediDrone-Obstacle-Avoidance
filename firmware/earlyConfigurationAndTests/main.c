#include "tmf8805.h"
#include <msp430.h>
#include <msp430fr5738.h>

// General Constants
#define MAX_BYTE 0xFF
#define ON 1
#define OFF 0

// tmf8805 RAM patch
#define RAM_PATCH_FLAG 1

int initializeTof();
int downloadRamPatch();

int startTof()
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

int initializeTof()
{
    unsigned char currentApp[READ_ONE_BYTE] = {MAX_BYTE};
    delay(10);
    while(currentApp[0] != BOOTLOADER && currentApp[0] != APP_ZERO)
    {
        performReadSequence(DISCOVER_RUNNING_APP_KEY, currentApp);
        delay(10);
    }

    int tofInitialized = 0;
    switch((currentApp[0])) {
        case BOOTLOADER:
            if (RAM_PATCH_FLAG)
                tofInitialized = downloadRamPatch();
            break;
        case APP_ZERO:
            ledState(ON);
            tofInitialized = 1;
            break;
        default:
            tofInitialized = 0;
    }
    return tofInitialized;
}

int resetTof()
{
    tofState(ON);
    performWriteSequence(RAM_RESET_KEY);
}

int downloadRamPatch()
{
    if (performWriteSequence(RAM_PATCH_KEY))
    {
        unsigned char currentApp[READ_ONE_BYTE] = {MAX_BYTE};
        delay(10);
        while(currentApp[0] != BOOTLOADER && currentApp[0] != APP_ZERO)
        {
            performReadSequence(DISCOVER_RUNNING_APP_KEY, currentApp);
            delay(10);
        }
        resetTof();
    }
    return 0;
}

/*
 * | BOARD # | PASS / FAIL |       TEST      |
 * |   2     |   PASS      |  init tof comm  |
 * |   3     |   PASS      |  init tof comm  |
 * |   5     |   PASS      |  init tof comm  |
 * |   4     |   PASS      |  init tof comm  |
 * |   1     |   FAIL      |  init tof comm  |
 */

int main(void)
{
    initializeGPIO();
    configureClocks(OFF, ON);
    initializeI2C();
    //resetTof();
    delay(50000);
    if (startTof())
        initializeTof();

    //_low_power_mode_0();
}
