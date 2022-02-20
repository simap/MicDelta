#ifndef __MICDATA_H
#define __MICDATA_H

#include "main.h"
#include "app.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int16_t input[ADC_BUF_SIZE];
	int16_t workingBuffer[ADC_BUF_SIZE];
	int16_t dcOffset;
	ADC_HandleTypeDef *adc;
} MicData;


HAL_StatusTypeDef micDataInit(MicData *md, ADC_HandleTypeDef *adc);

void micDataSnapshot(MicData *md);

void micDataProcess(MicData *md);


#ifdef __cplusplus
}
#endif

#endif



