#ifndef __WS2812DRIVER_H
#define __WS2812DRIVER_H

#include "main.h"
#include "app.h"



#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t bitBuffer[WS2812_DRIVER_BITBUFFER_SIZE];
	DMA_TypeDef * dma;
	uint32_t dmaChannel;
	uint32_t lastDrawTimerUs;
} WS2812Driver;

void ws2812DriverInit(WS2812Driver *wd, DMA_TypeDef * dma, uint32_t dmaChannel, USART_TypeDef * usart);

void ws2812DriverDraw(WS2812Driver *wd, uint8_t *pixels);

#ifdef __cplusplus
}
#endif

#endif

