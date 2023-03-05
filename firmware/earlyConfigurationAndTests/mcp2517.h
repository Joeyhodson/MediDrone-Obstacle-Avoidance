/*
 * mcp2517.h
 *
 *  Created on: Feb 27, 2023
 *      Author: Joeyh
 */

#include "./mcp251x/canfdspi/drv_canfdspi_api.h"
#include "./mcp251x/spi/drv_spi.h"

void basicCANConfiguration()
{
    // Reset Device
    DRV_CANFDSPI_Reset(DRV_CANFDSPI_INDEX_0);
    // Oscillator Configuration
    CAN_OSC_CTRL oscCtrl;
    DRV_CANFDSPI_OscillatorControlObjectReset(&oscCtrl);
    oscCtrl.OscDisable = 0;
    oscCtrl.ClkOutDivide = OSC_CLKO_DIV1;
    DRV_CANFDSPI_OscillatorControlSet(DRV_CANFDSPI_INDEX_0, oscCtrl);
    // Input/Output use nINT0 and nINT1
    //DRV_CANFDSPI_GpioModeConfigure(DRV_CANFDSPI_INDEX_0, GPIO_MODE_INT, GPIO_MODE_INT);
    // CAN Configuration: ISO_CRC, enable TEF, enable TXQ
    CAN_CONFIG canConfig;
    DRV_CANFDSPI_ConfigureObjectReset(&canConfig);
    canConfig.IsoCrcEnable = 0;
    canConfig.StoreInTEF = 0;
    canConfig.TXQEnable = 1;
    DRV_CANFDSPI_Configure(DRV_CANFDSPI_INDEX_0, &canConfig);
    // Bit Time Configuration: 500K/2M 80% sample point
    DRV_CANFDSPI_BitTimeConfigure(DRV_CANFDSPI_INDEX_0, CAN_500K_2M, CAN_SSP_MODE_AUTO, CAN_SYSCLK_20M);
    // TEF Configuration: 12 messages, time stamping enabled
    CAN_TEF_CONFIG tefConfig;
    tefConfig.FifoSize = 1; // = 11;
    tefConfig.TimeStampEnable = 1;
    DRV_CANFDSPI_TefConfigure(DRV_CANFDSPI_INDEX_0, &tefConfig);
    // TXQ Configuration: 8 messages, 32 byte maximum payload, high priority
    CAN_TX_QUEUE_CONFIG txqConfig;
    DRV_CANFDSPI_TransmitQueueConfigureObjectReset(&txqConfig);
    txqConfig.TxPriority = 1;
    txqConfig.FifoSize = 1; // = 7;
    txqConfig.PayLoadSize = CAN_PLSIZE_8;
    DRV_CANFDSPI_TransmitQueueConfigure(DRV_CANFDSPI_INDEX_0, &txqConfig);
    // FIFO 1: Transmit FIFO; 5 messages, 64 byte maximum payload, low priority
    CAN_TX_FIFO_CONFIG txfConfig;
    txfConfig.FifoSize = 4;
    txfConfig.PayLoadSize = CAN_PLSIZE_8;
    txfConfig.TxPriority = 0;
    DRV_CANFDSPI_TransmitChannelConfigure(DRV_CANFDSPI_INDEX_0, CAN_FIFO_CH1, &txfConfig);
    // FIFO 2: Receive FIFO; 16 messages, 64 byte maximum payload, time stamping enabled
    CAN_RX_FIFO_CONFIG rxfConfig;
    rxfConfig.FifoSize = 15;
    rxfConfig.PayLoadSize = CAN_PLSIZE_8;
    rxfConfig.RxTimeStampEnable = 1;
    DRV_CANFDSPI_ReceiveChannelConfigure(DRV_CANFDSPI_INDEX_0, CAN_FIFO_CH2, &rxfConfig);
}

// Now device is ready to transition to Normal Mode
// Enable ECC, Initialize RAM, select Normal Mode
void initializeRAMAndSelectNormalMode()
{
    // Enable Ecc
    int8_t err = DRV_CANFDSPI_EccEnable(DRV_CANFDSPI_INDEX_0);
    if (err == -1 || err == -2)
        ledState(ON);
    // Initialize RAM
    DRV_CANFDSPI_RamInit(DRV_CANFDSPI_INDEX_0, 0xff);
    // Configuration Done: Select Normal Mode
    DRV_CANFDSPI_OperationModeSelect(DRV_CANFDSPI_INDEX_0, CAN_NORMAL_MODE);
}

void transmitMessageFromTXFIFO()
{
    // Assemble transmit message: CAN FD Base Frame with BRS, 64 data bytes
    CAN_TX_MSGOBJ txObj;
    uint8_t txd[4];

    // Initialize ID and Control bits
    txObj.word[0] = 0;
    txObj.word[1] = 0;

    txObj.bF.id.SID = 0x300; // Standard or Base ID
    txObj.bF.id.EID = 0;

    txObj.bF.ctrl.FDF = 1; // CAN FD frame
    txObj.bF.ctrl.BRS = 1; // Switch bit rate
    txObj.bF.ctrl.IDE = 0; // Standard frame
    txObj.bF.ctrl.RTR = 0; // Not a remote frame request
    txObj.bF.ctrl.DLC = CAN_DLC_4; // 4 data bytes
    // Sequence: doesn't get transmitted, but will be stored in TEF
    txObj.bF.ctrl.SEQ = 1;

    // Initialize transmit data
    uint8_t i;
    for (i = 0; i < 4; i++) {
        txd[i] = 0xAA;
    }

    // Check that FIFO is not full
    CAN_TX_FIFO_EVENT txFlags;
    bool flush = true;

    int8_t err;
    err = DRV_CANFDSPI_TransmitChannelEventGet(DRV_CANFDSPI_INDEX_0, CAN_FIFO_CH1, &txFlags);
    if (txFlags & CAN_TX_FIFO_NOT_FULL_EVENT)
    {
        // Load message and transmit
        err = DRV_CANFDSPI_TransmitChannelLoad(DRV_CANFDSPI_INDEX_0, CAN_FIFO_CH1, &txObj, txd,
               /*DRV_CANFDSPI_DlcToDataBytes(txObj.bF.ctrl.DLC)*/4, flush);
    }

    if (err < 0)
        ledState(ON);
}

void readMessageFromTEF()
{
    // TEF Object
    CAN_TEF_MSGOBJ tefObj;
    uint32_t id;

    // Check that TEF is not empty
    CAN_TEF_FIFO_EVENT tefFlags;
    DRV_CANFDSPI_TefEventGet(DRV_CANFDSPI_INDEX_0, &tefFlags);

    if (tefFlags & CAN_TEF_FIFO_NOT_EMPTY_EVENT)
    {
        // Read message and UINC
        DRV_CANFDSPI_TefMessageGet(DRV_CANFDSPI_INDEX_0, &tefObj);
        // Process Message
        //Nop();
        //Nop();
        id = tefObj.bF.id.EID;
    }
}

void filterConfigurationToMatchAStandardFrameRange()
{
    // Configure Filter 0: match SID = 0x300-0x30F, Standard frames only
    // Disable Filter 0
    DRV_CANFDSPI_FilterDisable(DRV_CANFDSPI_INDEX_0, CAN_FILTER0);

    // Configure Filter Object 0
    CAN_FILTEROBJ_ID fObj;
    fObj.SID = 0x300; // IDs 0x300 onward
    fObj.SID11 = 0;
    fObj.EID = 0;
    fObj.EXIDE = 0; // only except Standard frames

    DRV_CANFDSPI_FilterObjectConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER0, &fObj);

    // COnfigure Mask Object 0
    CAN_MASKOBJ_ID mObj;
    mObj.MSID = 0x7F0; // 0 means dont care
    mObj.MSID11 = 0;
    mObj.MEID = 0;
    mObj.MIDE = 1; // match IDE bit

    DRV_CANFDSPI_FilterMaskConfigure(DRV_CANFDSPI_INDEX_0, CAN_FILTER0, &mObj);

    // Link Filter to RX FIFO 2, and enable filter
    bool filterEnable = true;
    DRV_CANFDSPI_FilterToFifoLink(DRV_CANFDSPI_INDEX_0, CAN_FILTER0, CAN_FIFO_CH2, filterEnable);

}

void receiveCANMessage()
{
    // Receive Message Object
    CAN_RX_MSGOBJ rxObj;
    uint8_t rxd[MAX_DATA_BYTES];

    // Check that FIFO is not empty
    CAN_RX_FIFO_EVENT rxFlags;

    DRV_CANFDSPI_ReceiveChannelEventGet(DRV_CANFDSPI_INDEX_0, CAN_FIFO_CH2, &rxFlags);

    if (rxFlags & CAN_RX_FIFO_NOT_EMPTY_EVENT)
    {
        // Read message and UINC
        DRV_CANFDSPI_ReceiveMessageGet(DRV_CANFDSPI_INDEX_0, CAN_FIFO_CH2, &rxObj, rxd, MAX_DATA_BYTES);

        // Process message
        if (rxObj.bF.id.SID==0x300 && rxObj.bF.ctrl.IDE==0)
        {
            //Nop();
            //Nop();
            return;
        }
    }
}

void configureTBC()
{
    // Disable TBC
    DRV_CANFDSPI_TimeStampDisable(DRV_CANFDSPI_INDEX_0);
    // COnfigure pre-scaler so TBC increments every 1 us @ 40Mhz Clock: 40-1 = 39
    DRV_CANFDSPI_TimeStampPrescalerSet(DRV_CANFDSPI_INDEX_0, 39);
    // Set TBC to zero
    DRV_CANFDSPI_TimeStampSet(DRV_CANFDSPI_INDEX_0, 0);
    // Enable TBC
    DRV_CANFDSPI_TimeStampEnable(DRV_CANFDSPI_INDEX_0);
}
