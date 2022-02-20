
#include "ws2812Driver.h"
#include "console.h"

extern uint8_t scanMode;


/*
use the UART serial stream to generate pulse widths of either 1 bit or
3 bits long, with 2 bits of spacing for 5 bits total. The start and stop
bits are also used, for 10 bits total, allowing 2 WS2812 bits per UART byte.
the UART output is inverted so that start bits (usually a low) are high
and stop bits are low.

At 4Mbaud each bit is 250ns, 1.25us for the 4-bit WS2812 bit with these timings:
T0H: 250ns T0L 1000ns
T1H: 750ns T1L: 500ns
*/

//this table lets us convert 2 bits at a time, MSB first
static const uint8_t bits[4] = {
	0b11101111, // out: -____-____
	0b10001111, // out: -____---__
	0b11101100, // out: ---__-____
	0b10001100, // out: ---__---__
};

void ws2812DriverInit(WS2812Driver *wd, DMA_TypeDef * dma, uint32_t dmaChannel, USART_TypeDef * usart) {
	wd->dma = dma;
	wd->dmaChannel = dmaChannel;
	wd->lastDrawTimerUs = 0;

	LL_DMA_SetMemoryAddress(dma, dmaChannel, (uint32_t) wd->bitBuffer);
	LL_DMA_SetPeriphAddress(dma, dmaChannel, (uint32_t) &usart->TDR);
	LL_USART_EnableDMAReq_TX(usart);

}


void ws2812DriverDraw(WS2812Driver *wd, uint8_t *pixels) {

	//ensure that previous DMA transfer was complete before overwriting buffer
	//and at least 250us has elapsed to allow WS2812 to latch before starting the next transfer
	//since these timings are constant,  ensure a fixed period of time has elapsed since the last call
	//at 4mbps each byte in the bitBuffer takes 2.5us, adding 250us for latch.
	const uint32_t microsPerFrame = (2.5 * WS2812_DRIVER_BITBUFFER_SIZE) + 250;

	while (micros() - wd->lastDrawTimerUs < microsPerFrame) {
		//wait
	}

	//fill the bit buffer for sending out pixel data over UART in WS2812 format
	uint32_t bbPos = 0;
	for (int pi = 0; pi < DISPLAY_BUFFER_SIZE; pi++) {
		//TODO map x,y to the matrix display
		uint8_t c = pixels[pi];
		//MSB first for WS2812 pixel bits
		wd->bitBuffer[bbPos++] = bits[c >> 6];
		wd->bitBuffer[bbPos++] = bits[(c >> 4) & 0x03];
		wd->bitBuffer[bbPos++] = bits[(c >> 2) & 0x03];
		wd->bitBuffer[bbPos++] = bits[c & 0x03];

	}

	//start the DMA transfer
	LL_DMA_DisableChannel(wd->dma, wd->dmaChannel);
	LL_DMA_SetDataLength(wd->dma, wd->dmaChannel, WS2812_DRIVER_BITBUFFER_SIZE);
	LL_DMA_EnableChannel(wd->dma, wd->dmaChannel);

	//in scan mode, show animation performance info
	if (scanMode) {
		uint32_t frameTime = micros() - wd->lastDrawTimerUs;
		ConsoleIoSendString("frame ");
		ConsoleSendParamInt32(frameTime);
		ConsoleIoSendString(" us");
		ConsoleIoSendString(STR_ENDLINE);
	}

	wd->lastDrawTimerUs = micros();
}
