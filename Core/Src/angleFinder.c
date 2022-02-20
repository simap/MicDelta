#include "angleFinder.h"


void angleFinderInit(AngleFinder *af, MicData *md1, MicData *md2) {
	af->md1 = md1;
	af->md2 = md2;
}

void angleFinderProcess(AngleFinder *af, q15_t *angleOutput, q15_t *strength) {
	//get micdata ouputs
	micDataProcess(af->md1);
	micDataProcess(af->md2);

	//correlate
	//TODO evaluate the faster variants: arm_correlate_fast_q15 and arm_correlate_opt_q15
	//TODO only a small part of the correlation is usable, check out the Partial Convolution APIs
	arm_correlate_q15(af->md1->workingBuffer, ADC_BUF_SIZE, af->md2->workingBuffer, ADC_BUF_SIZE, af->buffer);

	//find max, the position is most likely the phase offset of a sound
	q15_t maxValue;
	uint32_t maxIndex;
	arm_max_q15(af->buffer, ANGLEFINDER_BUFFER_SIZE, &maxValue, &maxIndex);

	//TODO use the max value along with overall signal levels to determine if there is a strong correlation, or just noise

	//TODO use delta compared to the maximum possible (based on the speed of sound between mics)

	//TODO calc angle, which is a bit more complicated

	//FIXME for now just return the delta from center to see what we get

	*angleOutput = (int16_t) maxIndex -  ANGLEFINDER_BUFFER_SIZE/2;
	*strength = maxValue;
}
