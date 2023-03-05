// Include files
#include "drv_spi.h"
//#include "helper.h"
/* Chip select. */
#define SPI_CHIP_SEL 0
//#define SPI_CHIP_PCS spi_get_pcs(SPI_CHIP_SEL)
#define SPI_NCS_PIN CHIP_SELECT
/* Clock polarity. */
#define SPI_CLK_POLARITY 0
/* Clock phase. */
#define SPI_CLK_PHASE 1
/* Delay before SPCK. */
#define SPI_DLYBS 0x00
/* Delay between consecutive transfers. */
#define SPI_DLYBCT 0x00
/* Clock Speed */
#define SPI_BAUDRATE 12500000

inline int8_t spi_master_transfer(uint8_t *SpiTxData, uint8_t *SpiRxData, uint16_t spiTransferSize);

void initializeSPI()
{
    UCA0CTL1 |= UCSWRST;
    // Configure ports to SPI functionality
    P2SEL1 |= (BIT0|BIT1);
    P2SEL0 &= ~(BIT0|BIT1);
    P2DIR |= (BIT2);
    P1SEL1 |= (BIT5);
    P1SEL0 &= ~(BIT5);
    // Keep all the default values except the fields below...
    // (UCMode 3:3-pin SPI [CS on non STE GPIO]) (Master Mode) (UCSSEL 1:ACLK, 2,3:SMCLK)
    UCA0CTLW0 |= (UCMODE_0|UCMST|UCSSEL__SMCLK|UCSYNC); // looks fine but ensure aclk is proper frequency
    // Clock divider = 1 (SMCLK @ ~1Mhz /  = ~1Mhz)
    UCA0BRW = 1;
    // Exit reset mode
    UCA0CTL1 &= ~UCSWRST;
    // Ensure all slave STE's are high
    P2OUT |= (BIT2);
}

void transmitMasterSPI(unsigned int txData)
{
    while (!(UCA0IFG & UCTXIFG)); // wait until transfer buffer ready
    UCA0TXBUF = txData;
}

void receiveMasterSPI(uint8_t *rxData, unsigned int position)
{
    //P2OUT |= (BIT2);
    while (!(UCA0IFG & UCRXIFG)); // wait until received
    rxData[position] = UCA0RXBUF;
    //P2OUT &= ~(BIT2);
}

void DRV_SPI_Initialize(void)
{
    initializeSPI();
}

int8_t DRV_SPI_TransferData(uint8_t spiSlaveDeviceIndex, uint8_t *SpiTxData, uint8_t *SpiRxData, uint16_t spiTransferSize)
{
    // specify slave device on SPI bus, if multiple MCP25xx on CAN node
    // we have 1 device per node hence the ignoring of spiSlaveDeviceIndex parameter
	return spi_master_transfer(SpiTxData, SpiRxData, spiTransferSize);
}

int8_t spi_master_transfer(uint8_t *SpiTxData, uint8_t *SpiRxData, uint16_t spiTransferSize)
{
	unsigned int position = 0;
	P2OUT &= ~(BIT2);
	while(position < spiTransferSize)
	{
	    transmitMasterSPI(SpiTxData[position]);
	    receiveMasterSPI(SpiRxData, position);
	    position++;
	}
	P2OUT |= (BIT2);
	return 0;
}

