#ifndef __DISPLAYVISUALIZER_H
#define __DISPLAYVISUALIZER_H

#include "main.h"
#include "app.h"
#include "angleFinder.h"


#define DISPLAY_BUFFER_WIDTH 8
#define DISPLAY_BUFFER_HEIGHT 8


#define DISPLAY_BUFFER_SIZE (DISPLAY_BUFFER_WIDTH * DISPLAY_BUFFER_HEIGHT * 3)


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	AngleFinder *afx;
	AngleFinder *afy;

	uint8_t displayBuffer[DISPLAY_BUFFER_SIZE];

} DisplayVisualizer;


void displayVisualizerInit(AngleFinder *afx, AngleFinder *afy);
void displayVisualizerProcess();



#ifdef __cplusplus
}
#endif

#endif

