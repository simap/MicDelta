#ifndef __DISPLAYVISUALIZER_H
#define __DISPLAYVISUALIZER_H

#include "main.h"
#include "app.h"
#include "angleFinder.h"
#include "ws2812Driver.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	AngleFinder *angleFinderX;
	AngleFinder *angleFinderY;
	WS2812Driver *ws2812Driver;

	uint8_t displayBuffer[DISPLAY_BUFFER_SIZE];

} DisplayVisualizer;


void displayVisualizerInit(DisplayVisualizer *dv, AngleFinder *angleFinderX, AngleFinder *angleFinderY, WS2812Driver * ws2812Driver);
void displayVisualizerProcess(DisplayVisualizer *dv);



#ifdef __cplusplus
}
#endif

#endif

