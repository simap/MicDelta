#ifndef __WS2812DRIVER_H
#define __WS2812DRIVER_H

#include "main.h"
#include "app.h"
#include "displayVisualization.h"

//need half a byte for each bit, and 24 bits
#define WS2812_DRIVER_BUFFER_SIZE (DISPLAY_BUFFER_SIZE * 24/2)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint8_t bitBuffer[WS2812_DRIVER_BUFFER_SIZE];
} WS2812Driver;



#ifdef __cplusplus
}
#endif

#endif

