#ifndef CONSOLEUART_H
#define CONSOLEUART_H

#include "stm32f3xx_ll_usart.h"
#include "stm32f3xx_ll_dma.h"


#define CONSOLEUART_TX_BUF 128
#define CONSOLEUART_RX_BUF 128

void initConsoleUart(USART_TypeDef *initUsart, DMA_TypeDef *initDma, uint32_t initTxDmaChannel, uint32_t initRxDmaChannel);

int consoleUartGetChNonBlocking();


void consoleUartTxDmaISR (); //install this in the appropriate transmit channel ISR
void consoleUartIsr();

#endif
