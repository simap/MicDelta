#ifndef __ANGLEFINDER_H
#define __ANGLEFINDER_H

#include "main.h"
#include "app.h"
#include "micData.h"


//used for correlate output. size: 2 * max(srcALen, srcBLen) - 1
#define ANGLEFINDER_BUFFER_SIZE (ADC_BUF_SIZE * 2 - 1)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int16_t buffer[ANGLEFINDER_BUFFER_SIZE];
	MicData *md1;
	MicData *md2;

} AngleFinder;


void angleFinderInit(AngleFinder *af, MicData *md1, MicData *md2);
void angleFinderProcess(AngleFinder *af, q15_t *angleOutput, q15_t *strength);


#ifdef __cplusplus
}
#endif

#endif

