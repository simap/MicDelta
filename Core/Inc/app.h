#ifndef __APP_H
#define __APP_H

#include <stdint.h>
#include <stdbool.h>
#include "arm_math.h"

#define FLIP_X_MICS 1
#define FLIP_Y_MICS 1

#define ADC_BUF_SIZE (512)

//used for correlation output. see docs for arm_correlate_q15
#define ANGLEFINDER_BUFFER_SIZE (ADC_BUF_SIZE * 2 - 1)

#define DISPLAY_BUFFER_WIDTH 8
#define DISPLAY_BUFFER_HEIGHT 8
#define DISPLAY_NUM_PIXELS (DISPLAY_BUFFER_WIDTH * DISPLAY_BUFFER_HEIGHT)
#define DISPLAY_BUFFER_SIZE (DISPLAY_NUM_PIXELS * 3)

#define VISUALIZATION_CORRELATION_THRESHOLD 15

//need half a byte for each bit, and 24 bits
#define WS2812_DRIVER_BITBUFFER_SIZE (DISPLAY_BUFFER_SIZE * 24/2)

#ifdef __cplusplus
extern "C" {
#endif

void startSampling();
void stopSampling();
uint32_t micros();
void delayUs(uint32_t us);

void appMain();


#ifdef __cplusplus
}
#endif

#endif
