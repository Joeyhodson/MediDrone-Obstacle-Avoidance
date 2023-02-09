// Information from TMF8X0X_Host_Driver_Communication by AMS
// Author: Joey Hodson
// 02/08/2023

// I2C Address
#define TMF8805_ADDRESS                   0x41

// Actions
#define WRITE                             0x01
#define READ                              0x00

// Names
#define BOOTLOADER                        0x80
#define APP_ZERO                          0xC0
#define RAM_PATCH_FLAG                    0x00

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

// I2C Sequence Sizes
#define WAKEUP_FROM_STANDBY_SIZE          0x00
#define PUT_INTO_STANDBY_SIZE             0x01
#define DISCOVER_RUNNING_APP_SIZE         0x02
#define DISCOVER_APP0_MAJOR_VERSION_SIZE  0x00
#define DISCOVER_APP0_MINOR_VERSION_SIZE  0x00
#define DISCOVER_ID_REVID_SIZE            0x00
#define START_SERIAL_NUMBER_SIZE          0x03
#define IS_SERIAL_NUMBER_READY_SIZE       0x00
#define READ_SERIAL_NUMBER_SIZE           0x00
#define IS_CPU_READY_SIZE                 0x00
#define START_CALIBRATION_SIZE            0x03
#define IS_CALIBRATION_READY_SIZE         0x0B
#define READ_CALIBRATION_DATA_SIZE        0x0C
#define CALIBRATE_APP0_SIZE               0x0D
#define START_APP0_SIZE                   0x0E
#define READ_RESULTS_SIZE                 0x0F
#define STOP_APP0_SIZE                    0x10

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

/*
 * Returns the i2c sequence given by the key map above.
 * Writes: | WRITE | REGISTER ADDRESS TO WRITE |         PAYLOAD         |
 * Reads:  | READ  | REGISTER ADDRESS TO READ  | NUMBER OF BYTES TO READ |
 */
unsigned char * getI2CSequence(unsigned char sequenceKey)
{
    static unsigned char const error[]                     = {-1};
    static unsigned char const wakeUpFromStandby[]         = {WRITE, REG_ENABLE,    WAKEUP_FROM_STANDBY_PL};
    static unsigned char const putIntoStandby[]            = {WRITE, REG_ENABLE,    PUT_INTO_STANDBY_PL};
    static unsigned char const startSerialNumber[]         = {WRITE, REG_SIXTEEN,   START_SERIAL_NUMBER_PL};
    static unsigned char const startCalibration[]          = {WRITE, REG_SIXTEEN,   START_CALIBRATION_PL};
    static unsigned char const stopApp0[]                  = {WRITE, REG_SIXTEEN,   STOP_APP0_PL};
    static unsigned char const calibrateApp0[]             = {WRITE, REG_CALIBRATE, CALIBRATE_APP0_PL_0,
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
    static unsigned char const startApp0[]                 = {WRITE, REG_EIGHT,     START_APP0_PL_0,
                                                                                    START_APP0_PL_1,
                                                                                    START_APP0_PL_2,
                                                                                    START_APP0_PL_3,
                                                                                    START_APP0_PL_4,
                                                                                    START_APP0_PL_5,
                                                                                    START_APP0_PL_6,
                                                                                    START_APP0_PL_7,
                                                                                    START_APP0_PL_8};
    static unsigned char const discoverRunningApp[]        = {READ,  REG_ZERO,      READ_ONE_BYTE};
    static unsigned char const discoverApp0MajorVersion[]  = {READ,  REG_MAJOR,     READ_ONE_BYTE};
    static unsigned char const discoverApp0MinorVersion[]  = {READ,  REG_MINOR,     READ_TWO_BYTE};
    static unsigned char const discoverIDREVID[]           = {READ,  REG_ID,        READ_TWO_BYTE};
    static unsigned char const isSerialNumberReady[]       = {READ,  REG_READY,     READ_ONE_BYTE};
    static unsigned char const readSerialNumber[]          = {READ,  REG_SERIAL,    READ_FOUR_BYTE};
    static unsigned char const isCpuReady[]                = {READ,  REG_ENABLE,    READ_TWO_BYTE};
    static unsigned char const isCalibrationReady[]        = {READ,  REG_READY,     READ_TWO_BYTE};
    static unsigned char const readCalibrationData[]       = {READ,  REG_CALIBRATE, READ_FOURTEEN_BYTE};
    static unsigned char const readResults[]               = {READ,  REG_RESULTS,   READ_ELEVEN_BYTE};

    switch(sequenceKey)
    {
        case WAKEUP_FROM_STANDBY_KEY:           return wakeUpFromStandby;
        case PUT_INTO_STANDBY_KEY:              return putIntoStandby;
        case DISCOVER_RUNNING_APP_KEY:          return discoverRunningApp;
        case DISCOVER_APP0_MAJOR_VERSION_KEY:   return discoverApp0MajorVersion;
        case DISCOVER_APP0_MINOR_VERSION_KEY:   return discoverApp0MinorVersion;
        case DISCOVER_ID_REVID_KEY:             return discoverIDREVID;
        case START_SERIAL_NUMBER_KEY:           return startSerialNumber;
        case IS_SERIAL_NUMBER_READY_KEY:        return isSerialNumberReady;
        case READ_SERIAL_NUMBER_KEY:            return readSerialNumber;
        case IS_CPU_READY_KEY:                  return isCpuReady;
        case START_CALIBRATION_KEY:             return startCalibration;
        case IS_CALIBRATION_READY_KEY:          return isCalibrationReady;
        case READ_CALIBRATION_DATA_KEY:         return readCalibrationData;
        case CALIBRATE_APP0_KEY:                return calibrateApp0;
        case START_APP0_KEY:                    return startApp0;
        case READ_RESULTS_KEY:                  return readResults;
        case STOP_APP0_KEY:                     return stopApp0;
        default:                                return error;
    }
    return error;
}

int getI2CSequenceSize(unsigned char sequenceKey)
{
    int error = -1;
    switch(sequenceKey)
    {
        case WAKEUP_FROM_STANDBY_KEY:           return wakeUpFromStandby;
        case PUT_INTO_STANDBY_KEY:              return putIntoStandby;
        case DISCOVER_RUNNING_APP_KEY:          return discoverRunningApp;
        case DISCOVER_APP0_MAJOR_VERSION_KEY:   return discoverApp0MajorVersion;
        case DISCOVER_APP0_MINOR_VERSION_KEY:   return discoverApp0MinorVersion;
        case DISCOVER_ID_REVID_KEY:             return discoverIDREVID;
        case START_SERIAL_NUMBER_KEY:           return startSerialNumber;
        case IS_SERIAL_NUMBER_READY_KEY:        return isSerialNumberReady;
        case READ_SERIAL_NUMBER_KEY:            return readSerialNumber;
        case IS_CPU_READY_KEY:                  return isCpuReady;
        case START_CALIBRATION_KEY:             return startCalibration;
        case IS_CALIBRATION_READY_KEY:          return isCalibrationReady;
        case READ_CALIBRATION_DATA_KEY:         return readCalibrationData;
        case CALIBRATE_APP0_KEY:                return calibrateApp0;
        case START_APP0_KEY:                    return startApp0;
        case READ_RESULTS_KEY:                  return readResults;
        case STOP_APP0_KEY:                     return stopApp0;
        default:                                return error;
    }
    return error;
}

void doSequence(unsigned char sequenceKey)
{
    int
    int mySequence[getI2CSequenceSize(sequenceKey)] = getI2CSequence(sequenceKey);
}
