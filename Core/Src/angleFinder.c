#include "angleFinder.h"
#include "console.h"
#include "consoleio.h"

void angleFinderInit(AngleFinder *af, MicData *md1, MicData *md2) {
	af->md1 = md1;
	af->md2 = md2;
}

void angleFinderProcess(AngleFinder *af) {
	uint32_t startTime = micros();
	//get micdata ouputs
	micDataProcess(af->md1);
	micDataProcess(af->md2);

	//correlate
	//TODO evaluate the faster variants: arm_correlate_fast_q15 and arm_correlate_opt_q15
	//TODO only a small part of the correlation is usable, check out the Partial Convolution APIs
	arm_correlate_q15(af->md1->workingBuffer, ADC_BUF_SIZE, af->md2->workingBuffer, ADC_BUF_SIZE, af->buffer);

	//find max, the position is most likely the phase offset of a sound
	//TODO it should be possible to find a function that fits the correlated peak, and interpolate fractional deltas
	q15_t maxValue;
	uint32_t maxIndex;
	arm_max_q15(af->buffer, ANGLEFINDER_BUFFER_SIZE, &maxValue, &maxIndex);


	//TODO calc angle, which is a bit more complicated
	//for now just use the delta from center
	af->angle = (int16_t) maxIndex -  ANGLEFINDER_BUFFER_SIZE/2;
	af->strength = maxValue;

	af->lastProcessTimeUs = micros() - startTime;
}

void angleFinderDumpToConsole(AngleFinder *af) {
	ConsoleIoSendString("correlation data: ");
	ConsoleIoSendString(STR_ENDLINE);
	ConsoleIoSendString("mic1\tmic2\tcorr");
	ConsoleIoSendString(STR_ENDLINE);
	for (int i = 0; i < ADC_BUF_SIZE; i++) {
		ConsoleSendParamInt16(af->md1->workingBuffer[i]);
		ConsoleIoSendString("\t");
		ConsoleSendParamInt16(af->md2->workingBuffer[i]);
		ConsoleIoSendString("\t");
		// only show the middle bit of the correlation
		ConsoleSendParamInt16(af->buffer[i + ADC_BUF_SIZE/2]);
		ConsoleIoSendString(STR_ENDLINE);
	}

	ConsoleIoSendString("Angle found: ");
	ConsoleSendParamInt16(af->angle);
	ConsoleIoSendString(" strength ");
	ConsoleSendParamInt16(af->strength);
	ConsoleIoSendString(" in ");
	ConsoleSendParamInt32(af->lastProcessTimeUs);
	ConsoleIoSendString(" us.");
	ConsoleIoSendString(STR_ENDLINE);
}
