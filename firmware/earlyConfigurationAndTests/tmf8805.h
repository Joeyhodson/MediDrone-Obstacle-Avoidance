// Information from TMF8X0X_Host_Driver_Communication by AMS
// Author: Joey Hodson
// 02/08/2023

#include <msp430.h>
#include <msp430fr5738.h>

#include "helper.h"
#include "ramPatch.h"

// Macros
#define LENGTH(array) ((unsigned char) (sizeof(array) / sizeof(array)[0]))
// General
#define MAX_BYTE                          0xFF
// Write Sequence Lengths
#define DEFAULT_WRITE_SEQUENCE_SIZE       0x03
// I2C Address
#define TMF8805_ADDRESS                   0x41
// Actions
#define WRITE                             0x01
#define READ                              0x00
// Names
#define BOOTLOADER                        0x80
#define APP_ZERO                          0xC0
// Registers
#define REG_ENABLE                        0xE0
#define REG_ZERO                          0x00
#define REG_MAJOR                         0x01
#define REG_EIGHT                         0x08
#define REG_SIXTEEN                       0x10
#define REG_MINOR                         0x12
#define REG_ID                            0xE3
#define REG_READY                         0x1E
#define REG_SERIAL                        0x28
#define REG_CALIBRATE                     0x20
#define REG_RESULTS                       0x1D
// I2C Sequence Payload Writes
#define WAKEUP_FROM_STANDBY_PL            0x01
#define PUT_INTO_STANDBY_PL               0x00
#define START_SERIAL_NUMBER_PL            0x2F
#define START_CALIBRATION_PL              0x0A
#define CALIBRATE_APP0_PL_0               0x01
#define CALIBRATE_APP0_PL_1               0x17
#define CALIBRATE_APP0_PL_2               0x00
#define CALIBRATE_APP0_PL_3               0xFF
#define CALIBRATE_APP0_PL_4               0x04
#define CALIBRATE_APP0_PL_5               0x20
#define CALIBRATE_APP0_PL_6               0x40
#define CALIBRATE_APP0_PL_7               0x80
#define CALIBRATE_APP0_PL_8               0x00
#define CALIBRATE_APP0_PL_9               0x01
#define CALIBRATE_APP0_PL_10              0x02
#define CALIBRATE_APP0_PL_11              0x04
#define CALIBRATE_APP0_PL_12              0x00
#define CALIBRATE_APP0_PL_13              0xFC
#define START_APP0_PL_0                   0x03
#define START_APP0_PL_1                   0x23
#define START_APP0_PL_2                   0x00
#define START_APP0_PL_3                   0x00
#define START_APP0_PL_4                   0x00
#define START_APP0_PL_5                   0x64
#define START_APP0_PL_6                   0xD8
#define START_APP0_PL_7                   0x04
#define START_APP0_PL_8                   0x02
#define STOP_APP0_PL                      0x01
#define DOWNLOAD_INIT_PL_0                0x14
#define DOWNLOAD_INIT_PL_1                0x01
#define DOWNLOAD_INIT_PL_2                0x29
#define DOWNLOAD_INIT_PL_3                0xC1
#define SET_ADDRESS_POINTER_PL_0          0x43
#define SET_ADDRESS_POINTER_PL_1          0x02
#define SET_ADDRESS_POINTER_PL_2          0x00
#define SET_ADDRESS_POINTER_PL_3          0x00
#define SET_ADDRESS_POINTER_PL_4          0xBA
#define RAM_REMAP_RESET_PL_0              0x11
#define RAM_REMAP_RESET_PL_1              0x00
#define RAM_REMAP_RESET_PL_2              0xEE
#define RAM_RESET_PL_0                    0x10
#define RAM_RESET_PL_1                    0x00
#define RAM_RESET_PL_2                    0xEF
// I2C Sequence Payload Reads
#define READ_ONE_BYTE                     0x01
#define READ_TWO_BYTE                     0x02
#define READ_THREE_BYTE                   0x03
#define READ_FOUR_BYTE                    0x04
#define READ_ELEVEN_BYTE                  0x0B
#define READ_FOURTEEN_BYTE                0x0E
// I2C Sequence Keys
#define WAKEUP_FROM_STANDBY_KEY           0x00 // "S 41 W E0 01 P"
#define PUT_INTO_STANDBY_KEY              0x01 // "S 41 W E0 00 P"
#define DISCOVER_RUNNING_APP_KEY          0x02 // "S 41 W 00 Sr 41 R N P"
#define DISCOVER_APP0_MAJOR_VERSION_KEY   0x03 // "S 41 W 01 Sr 41 R N P"
#define DISCOVER_APP0_MINOR_VERSION_KEY   0x04 // "S 41 W 12 Sr 41 R A N P"
#define DISCOVER_ID_REVID_KEY             0x05 // "S 41 W E3 Sr 41 R A N P"
#define START_SERIAL_NUMBER_KEY           0x06 // "S 41 W 10 47 P"
#define IS_SERIAL_NUMBER_READY_KEY        0x07 // "S 41 W 1E Sr 41 R N P"
#define READ_SERIAL_NUMBER_KEY            0x08 // "S 41 W 28 Sr 41 R A A A N P"
#define IS_CPU_READY_KEY                  0x09 // "S 41 W E0 Sr 41 R A P"
#define START_CALIBRATION_KEY             0x0A // "S 41 W 10 0A P"
#define IS_CALIBRATION_READY_KEY          0x0B // "S 41 W 1E Sr 41 R A N P"
#define READ_CALIBRATION_DATA_KEY         0x0C // "S 41 W 20 Sr 41 R A A A A A A A A A A A A A N P"
#define CALIBRATE_APP0_KEY                0x0D // "S 41 W 20 01 17 00 ff 04 20 40 80 00 01 02 04 00 fc P"
#define START_APP0_KEY                    0x0E // "S 41 W 08 03 23 00 00 00 64 D8 04 02 P"
#define READ_RESULTS_KEY                  0x0F // "S 41 W 1D Sr 41 R A A A A A A A A A A N P"
#define STOP_APP0_KEY                     0x10 // "S 41 W 10 ff P"
#define DOWNLOAD_INIT_KEY                 0x11 // "S 41 W 08 14 01 29 C1 P"
#define READ_STATUS_KEY                   0x12 // "S 41 W 08 Sr 41 R A A N P"
#define SET_ADDRESS_POINTER_KEY           0x13 // "S 41 W 08 43 02 00 00 BA P"
#define RAM_PATCH_KEY                     0x14 // "S 41 W 08 41 10 6D C9 41 85 3D 15 AA 51 F4 D2 9E A8 A7 AC 77 E9 A6 P" (example)
#define RAM_REMAP_RESET_KEY               0x15 // "S 41 W 08 11 00 EE P"
#define RAM_RESET_KEY                     0x16 // "S 41 W 08 10 00 EF p"

static unsigned char wakeUpFromStandby[]         = {WRITE, REG_ENABLE,    WAKEUP_FROM_STANDBY_PL};
static unsigned char putIntoStandby[]            = {WRITE, REG_ENABLE,    PUT_INTO_STANDBY_PL};
static unsigned char startSerialNumber[]         = {WRITE, REG_SIXTEEN,   START_SERIAL_NUMBER_PL};
static unsigned char startCalibration[]          = {WRITE, REG_SIXTEEN,   START_CALIBRATION_PL};
static unsigned char stopApp0[]                  = {WRITE, REG_SIXTEEN,   STOP_APP0_PL};
static unsigned char calibrateApp0[]             = {WRITE, REG_CALIBRATE, CALIBRATE_APP0_PL_0,
                                                                          CALIBRATE_APP0_PL_1,
                                                                          CALIBRATE_APP0_PL_2,
                                                                          CALIBRATE_APP0_PL_3,
                                                                          CALIBRATE_APP0_PL_4,
                                                                          CALIBRATE_APP0_PL_5,
                                                                          CALIBRATE_APP0_PL_6,
                                                                          CALIBRATE_APP0_PL_7,
                                                                          CALIBRATE_APP0_PL_8,
                                                                          CALIBRATE_APP0_PL_9,
                                                                          CALIBRATE_APP0_PL_10,
                                                                          CALIBRATE_APP0_PL_11,
                                                                          CALIBRATE_APP0_PL_12,
                                                                          CALIBRATE_APP0_PL_13};
static unsigned char startApp0[]                 = {WRITE, REG_EIGHT,     START_APP0_PL_0,
                                                                          START_APP0_PL_1,
                                                                          START_APP0_PL_2,
                                                                          START_APP0_PL_3,
                                                                          START_APP0_PL_4,
                                                                          START_APP0_PL_5,
                                                                          START_APP0_PL_6,
                                                                          START_APP0_PL_7,
                                                                          START_APP0_PL_8};
static unsigned char downloadInit[]              = {WRITE, REG_EIGHT,     DOWNLOAD_INIT_PL_0,
                                                                          DOWNLOAD_INIT_PL_1,
                                                                          DOWNLOAD_INIT_PL_2,
                                                                          DOWNLOAD_INIT_PL_3};
static unsigned char setAddressPointer[]         = {WRITE, REG_EIGHT,     SET_ADDRESS_POINTER_PL_0,
                                                                          SET_ADDRESS_POINTER_PL_1,
                                                                          SET_ADDRESS_POINTER_PL_2,
                                                                          SET_ADDRESS_POINTER_PL_3,
                                                                          SET_ADDRESS_POINTER_PL_4};
static unsigned char ramRemapReset[]             = {WRITE, REG_EIGHT,     RAM_REMAP_RESET_PL_0,
                                                                          RAM_REMAP_RESET_PL_1,
                                                                          RAM_REMAP_RESET_PL_2};
static unsigned char ramReset[]                  = {WRITE, REG_EIGHT,     RAM_RESET_PL_0,
                                                                          RAM_RESET_PL_1,
                                                                          RAM_RESET_PL_2};
static unsigned char discoverRunningApp[]        = {READ,  REG_ZERO,      READ_ONE_BYTE};
static unsigned char discoverApp0MajorVersion[]  = {READ,  REG_MAJOR,     READ_ONE_BYTE};
static unsigned char discoverApp0MinorVersion[]  = {READ,  REG_MINOR,     READ_TWO_BYTE};
static unsigned char discoverIDREVID[]           = {READ,  REG_ID,        READ_TWO_BYTE};
static unsigned char isSerialNumberReady[]       = {READ,  REG_READY,     READ_ONE_BYTE};
static unsigned char readSerialNumber[]          = {READ,  REG_SERIAL,    READ_FOUR_BYTE};
static unsigned char isCpuReady[]                = {READ,  REG_ENABLE,    READ_ONE_BYTE};
static unsigned char isCalibrationReady[]        = {READ,  REG_READY,     READ_TWO_BYTE};
static unsigned char readCalibrationData[]       = {READ,  REG_CALIBRATE, READ_FOURTEEN_BYTE};
static unsigned char readResults[]               = {READ,  REG_RESULTS,   READ_ELEVEN_BYTE};
static unsigned char readStatus[]                = {READ,  REG_EIGHT,     READ_THREE_BYTE};

int i2cWriteBytesToRegister(const unsigned char i2cAddress, const unsigned char i2cRegister, const unsigned char* payload, int payloadSize);
int i2cReadBytesFromRegister(unsigned char i2cAddress, unsigned char i2cRegister, unsigned char bytesToRead, unsigned char* dataBack);
int performWriteSequence(unsigned char sequenceKey);
int performReadSequence(unsigned char sequenceKey, unsigned char* dataBack);
int ramPatchReadStatusHelper();
int performRamPatch();

int performWriteSequence(unsigned char sequenceKey)
{
    unsigned char *sequence;
    int sequenceSize = DEFAULT_WRITE_SEQUENCE_SIZE;
    switch(sequenceKey)
    {
        case WAKEUP_FROM_STANDBY_KEY:           sequence = wakeUpFromStandby; break;
        case PUT_INTO_STANDBY_KEY:              sequence = putIntoStandby; break;
        case START_SERIAL_NUMBER_KEY:           sequence = startSerialNumber; break;
        case START_CALIBRATION_KEY:             sequence = startCalibration; break;
        case STOP_APP0_KEY:                     sequence = stopApp0; break;
        case RAM_REMAP_RESET_KEY:               sequence = ramRemapReset; sequenceSize = 5; break;
        case CALIBRATE_APP0_KEY:                sequence = calibrateApp0; sequenceSize = 16; break;
        case START_APP0_KEY:                    sequence = startApp0; sequenceSize = 11; break;
        case DOWNLOAD_INIT_KEY:                 sequence = downloadInit; sequenceSize = 6; break;
        case SET_ADDRESS_POINTER_KEY:           sequence = setAddressPointer; sequenceSize = 7; break;
        case RAM_RESET_KEY:                     sequence = ramReset; sequenceSize = 5; break;
        case RAM_PATCH_KEY:                     return performRamPatch();
        default:                                return 0;
    }
    return i2cWriteBytesToRegister(TMF8805_ADDRESS, sequence[1], sequence + 2, sequenceSize - 2);
}

int performReadSequence(unsigned char sequenceKey, unsigned char* dataBack)
{
    unsigned char *sequence;
    switch(sequenceKey)
    {
        case DISCOVER_RUNNING_APP_KEY:          sequence = discoverRunningApp; break;
        case DISCOVER_APP0_MAJOR_VERSION_KEY:   sequence = discoverApp0MajorVersion; break;
        case DISCOVER_APP0_MINOR_VERSION_KEY:   sequence = discoverApp0MinorVersion; break;
        case DISCOVER_ID_REVID_KEY:             sequence = discoverIDREVID; break;
        case IS_SERIAL_NUMBER_READY_KEY:        sequence = isSerialNumberReady; break;
        case READ_SERIAL_NUMBER_KEY:            sequence = readSerialNumber; break;
        case IS_CPU_READY_KEY:                  sequence = isCpuReady; break;
        case IS_CALIBRATION_READY_KEY:          sequence = isCalibrationReady; break;
        case READ_CALIBRATION_DATA_KEY:         sequence = readCalibrationData; break;
        case READ_RESULTS_KEY:                  sequence = readResults; break;
        case READ_STATUS_KEY:                   sequence = readStatus; break;
        default:                                return 0;
    }
    return i2cReadBytesFromRegister(TMF8805_ADDRESS, sequence[1], sequence[2], dataBack);
}

unsigned char* combineArray(const unsigned char* recordHead, const unsigned char* recordTail)
{
    unsigned char combinedArray[20];
    unsigned int currentByte;
    for (currentByte = 0; currentByte < 3; currentByte++)
        combinedArray[currentByte] = recordHead[currentByte];
    for (currentByte = 0; currentByte < 17; currentByte++)
        combinedArray[currentByte+3] = recordTail[currentByte];

    return combinedArray;
}

int performRamPatch()
{
    performWriteSequence(DOWNLOAD_INIT_KEY);
    ramPatchReadStatusHelper();
    performWriteSequence(SET_ADDRESS_POINTER_KEY);
    int lastRecord = 728;//LENGTH(mainPatchRecords);
    const int sequenceSize = 21;
    unsigned char* fullRecord;
    unsigned int record;
    for (record = 0; record < lastRecord; record++)
    {
        fullRecord = combineArray(basePatch, mainPatchRecords[record]);
        delay(10);
        if(!i2cWriteBytesToRegister(TMF8805_ADDRESS, fullRecord[0], fullRecord + 1, sequenceSize - 2))
            return 0;
        if(!ramPatchReadStatusHelper())
            return 0;
    }
    delay(10);
    performWriteSequence(RAM_REMAP_RESET_KEY);
    return 1;
}

int ramPatchReadStatusHelper()
{
    unsigned char readStatus[READ_THREE_BYTE] = {MAX_BYTE, 0, 0xFF};
    while(readStatus[2] == MAX_BYTE)
    {
        delay(10);
        performReadSequence(READ_STATUS_KEY, readStatus);
    }

    return 1;
}

int i2cWriteBytesToRegister(const unsigned char i2cAddress, const unsigned char i2cRegister, const unsigned char* payload, int payloadSize)
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

    // Stage transmit buffer with i2c register
    UCB0TXBUF = i2cRegister;
    // wait for start to clear
    while ((UCB0CTLW0 & UCTXSTT) != 0) {}
    // added this now works to write register
    // wait for transmit buffer flag to clear
    while ((UCB0IFG & UCTXIFG0) == 0) {}

    // write all bytes
    // might have to add ack in between every write
    for (int currentByte = 0; currentByte < payloadSize; currentByte++)
    {
        UCB0TXBUF = payload[currentByte];
        while ((UCB0IFG & UCTXIFG0) == 0) {}
    }

    // Setup the stop signal
    UCB0CTLW0 |= UCTXSTP;
    while ((UCB0CTLW0 & UCTXSTP) != 0) {}

    return 1;
}

int i2cReadBytesFromRegister(unsigned char i2cAddress, unsigned char i2cRegister, unsigned char bytesToRead, unsigned char* dataBack)
{
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
    for (int currentByte = 0; currentByte < bytesToRead; currentByte++)
    {
        while ((UCB0IFG & UCRXIFG0) == 0) {}
        dataBack[currentByte] = UCB0RXBUF;
    }

    // Setup the stop signal
    UCB0CTLW0 |= UCTXSTP;
    while ((UCB0CTLW0 & UCTXSTP) != 0) {}

    return 1;
}
