#ifndef __PIXELMAP_H
#define __PIXELMAP_H

#include "main.h"
#include "app.h"


#ifdef __cplusplus
extern "C" {
#endif

uint32_t getDisplayPixelOffset(uint16_t x, uint16_t y);

uint32_t getOutputPixelOffset(uint16_t x, uint16_t y);


#ifdef __cplusplus
}
#endif

#endif


