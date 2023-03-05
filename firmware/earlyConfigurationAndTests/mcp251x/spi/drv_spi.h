#ifndef _DRV_SPI_H
#define _DRV_SPI_H

// Include files
#include <stdint.h>
#include <msp430.h>
#include <msp430fr5738.h>

// Index to SPI channel
// Used when multiple MCP25xxFD are connected to the same SPI interface, but with different CS
#define DRV_CANFDSPI_INDEX_0         0
#define DRV_CANFDSPI_INDEX_1         1

// Index to SPI channel
// Used when multiple MCP25xxFD are connected to the same SPI interface, but with different CS
#define SPI_DEFAULT_BUFFER_LENGTH 96

//! SPI Initialization

void DRV_SPI_Initialize(void);

//! SPI Read/Write Transfer

void initializeSPI();
void transmitMasterSPI(unsigned int* data);
void receiveMasterSPI(unsigned int rxData, unsigned int position);
int8_t DRV_SPI_TransferData(uint8_t spiSlaveDeviceIndex, uint8_t *SpiTxData, uint8_t *SpiRxData, uint16_t spiTransferSize);

#endif	// _DRV_SPI_H
