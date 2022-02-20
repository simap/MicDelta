#ifndef __ANGLEFINDER_H
#define __ANGLEFINDER_H

#include "main.h"
#include "app.h"
#include "micData.h"


#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int16_t buffer[ANGLEFINDER_BUFFER_SIZE];
	MicData *md1;
	MicData *md2;
	q15_t angle;
	q15_t strength;
	uint32_t lastProcessTimeUs;
} AngleFinder;


void angleFinderInit(AngleFinder *af, MicData *md1, MicData *md2);
void angleFinderProcess(AngleFinder *af);
void angleFinderDumpToConsole(AngleFinder *af);

#ifdef __cplusplus
}
#endif

#endif

