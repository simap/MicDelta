
#include "consoleUart.h"
#include "main.h"
#include <stdio.h>


//ping pong between 2 tx buffers
uint8_t txBufferPool[2][CONSOLEUART_TX_BUF];
volatile uint8_t *txBuf = txBufferPool[0];
volatile uint16_t txPos = 0;

volatile static int uartErrors; //for debugging


uint8_t rxBuf[CONSOLEUART_RX_BUF];
uint16_t rxPos = 0;


USART_TypeDef *usart;
DMA_TypeDef *dma;
uint32_t txDmaChannel;
uint32_t rxDmaChannel;

/**
 *
 * channels like LL_DMA_CHANNEL_1
 */
void initConsoleUart(USART_TypeDef *initUsart, DMA_TypeDef *initDma, uint32_t initTxDmaChannel, uint32_t initRxDmaChannel) {

	//save this info for later
	usart = initUsart;
	dma = initDma;
	txDmaChannel = initTxDmaChannel;
	rxDmaChannel = initRxDmaChannel;


	//listen for errors via interrupt
	LL_USART_EnableIT_ERROR(usart);

	uint32_t rxAddr = LL_USART_DMA_GetRegAddr(usart, LL_USART_DMA_REG_DATA_RECEIVE);
	//NOTE this assumes the basics have been set up by the generated LL drivers
	LL_DMA_ConfigAddresses(dma, rxDmaChannel, rxAddr, (uint32_t) rxBuf, LL_DMA_DIRECTION_PERIPH_TO_MEMORY);
	LL_DMA_SetDataLength(dma, rxDmaChannel, CONSOLEUART_RX_BUF);
	LL_DMA_EnableChannel(dma, rxDmaChannel);

	//TX addresses will be set up when a transfer is in progress


	LL_USART_EnableDMAReq_RX(usart);
	LL_USART_EnableDMAReq_TX(usart);
}


void consoleUartIsr() {
	//check all the uart error conditions
	if (USART1->ISR & (USART_ISR_FE | USART_ISR_ORE | USART_ISR_NE)) {
		//we don't really care to handle the error in any special way
		//this is more for debugging purposes and to clear the error bits
		uartErrors++;
		//to clear these errors, write 1 to the appropriate bits in USART_ICR
		USART1->ICR |= USART_ICR_FECF | USART_ICR_ORECF | USART_ICR_NCF;
	}
}


// ===== RX =====

int consoleUartRxAvailable() {

	int res = (CONSOLEUART_RX_BUF - LL_DMA_GetDataLength(dma, rxDmaChannel)) - rxPos;
	if (res < 0)
		res += CONSOLEUART_RX_BUF;
	return res;
}

int consoleUartGetChNonBlocking() {
	if (rxPos == (CONSOLEUART_RX_BUF - LL_DMA_GetDataLength(dma, rxDmaChannel)))
		return EOF;
	uint8_t res = rxBuf[rxPos++];
	if (rxPos >= CONSOLEUART_RX_BUF)
		rxPos = 0;
	return res;
}

//hook in to the syscalls code. so scanf (I guess?) works
int __io_getchar(void) {
	int res;
	do {
		res = consoleUartGetChNonBlocking();
	} while (res == EOF);
	return res;
}


// ===== TX =====

static void startDmaTx() {
	if (txPos == 0)
		return;


	//fire up DMA to send what's in the current buffer
	uint32_t txAddr = LL_USART_DMA_GetRegAddr(usart, LL_USART_DMA_REG_DATA_TRANSMIT);
	LL_DMA_ConfigAddresses(dma, txDmaChannel, (uint32_t) txBuf, txAddr, LL_DMA_DIRECTION_MEMORY_TO_PERIPH);
	LL_DMA_SetDataLength(dma, txDmaChannel, txPos);

	//swap buffers and reset txBuf BEFORE starting DMA, or it could fire interrupt before this happens
	txPos = 0;
	txBuf = txBuf == txBufferPool[0] ? txBufferPool[1] : txBufferPool[0];

	LL_DMA_EnableIT_TC(dma, txDmaChannel);
	LL_DMA_EnableChannel(dma, txDmaChannel);

}

static void startDmaTxIfNeeded() {
	//if DMA is enabled, there's a transfer in progress, don't start another!
	if (LL_DMA_IsEnabledChannel(dma, txDmaChannel))
		return;
	startDmaTx();
}

void consoleUartTxDmaISR () {

	//find the TCIF bit for our channel. There are 4 bits per channel, and TCIF is one over from the base
	//there isn't a LL driver API for this based on channel
	uint32_t tcifBit = 1 << ((txDmaChannel-1) * 4 + 1);
	WRITE_REG(dma->IFCR, tcifBit);
	//disable the dma channel so we can update it for the next transfer
	LL_DMA_DisableChannel(dma, txDmaChannel);
	//start the next transfer if there's any data available
	startDmaTx();
}


int consoleUartTxAvailable() {
	return CONSOLEUART_TX_BUF - txPos;
}


//hook in to the syscalls code so printf works
//TODO implement a _write that does multiple chars at a time (less overhead)
int __io_putchar(int ch) {
	//wait for a fresh buffer if necessary
	while (consoleUartTxAvailable() < 1) {
		//wait
	}

	LL_DMA_DisableIT_TC(dma, txDmaChannel);
	//don't let the ISR catch us mid write, we want txPos and data updates to happen atomically
	txBuf[txPos++] = ch;
	LL_DMA_EnableIT_TC(dma, txDmaChannel);

	startDmaTxIfNeeded();
	return ch;
}

