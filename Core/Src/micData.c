#include "micData.h"



HAL_StatusTypeDef micDataInit(MicData *md, ADC_HandleTypeDef *adc) {
	md->dcOffset = 1900;
	md->adc = adc;
	return HAL_ADC_Start_DMA(adc, (uint32_t *) &md->input, ADC_BUF_SIZE);
}

void micDataSnapshot(MicData *md) {
	//TODO maybe mem2mem dma would be faster? or use a pingpong buffer instead of circular
	//unwrap the circular buffer based on where DMA is writing
	uint32_t dmaCndtr = md->adc->DMA_Handle->Instance->CNDTR;
	int startIndex = ADC_BUF_SIZE - dmaCndtr;
	//copy what we can starting at startIndex
	if (ADC_BUF_SIZE - startIndex > 0) {
		memcpy(&md->workingBuffer[0], &md->input[startIndex],
				(ADC_BUF_SIZE - startIndex) * sizeof(q15_t));
	}
	//copy the rest (unless it was aligned to the beginning)
	if (startIndex != 0) {
		memcpy(&md->workingBuffer[ADC_BUF_SIZE - startIndex], &md->input[0],
				startIndex * sizeof(q15_t));
	}
}

void micDataProcess(MicData *md) {

	//find the DC component offset (average) for this chunk
	q15_t newOffset;
	arm_mean_q15(md->workingBuffer, ADC_BUF_SIZE, &newOffset);
	if (newOffset != md->dcOffset) {
		//nudge the channel dcOffset if needed, this will bounce around 1 LSB
		//TODO could store dcOffset with more resolution, filter, running average, etc.
		if (newOffset > md->dcOffset)
			md->dcOffset++;
		else
			md->dcOffset--;
	}

	//remove the DC offset and write to the output buffer
	arm_offset_q15(md->workingBuffer, -md->dcOffset, md->workingBuffer, ADC_BUF_SIZE);

	//signals are a bit weak, boost them
	arm_shift_q15(md->workingBuffer, 4, md->workingBuffer, ADC_BUF_SIZE);
}
