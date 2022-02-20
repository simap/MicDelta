#include "main.h"

#include "micData.h"
#include "angleFinder.h"
#include "ws2812Driver.h"
#include "displayVisualization.h"
#include "consoleUart.h"
#include "console.h"
#include "consoleio.h"
#include <stdio.h>

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;
extern ADC_HandleTypeDef hadc4;
extern DMA_HandleTypeDef hdma_adc1;
extern DMA_HandleTypeDef hdma_adc2;
extern DMA_HandleTypeDef hdma_adc3;
extern DMA_HandleTypeDef hdma_adc4;

extern OPAMP_HandleTypeDef hopamp1;
extern OPAMP_HandleTypeDef hopamp2;
extern OPAMP_HandleTypeDef hopamp3;
extern OPAMP_HandleTypeDef hopamp4;


/*
 * a snap has a big pulse about 2ms, with trailing signal. about 400hz, 2khz, 20khz.
 * with mics 260-300mm apart, get a .7ms delay
 * sample at about 50khz, will get 5.12ms in 256 samples, 2.56ms in 128
 * NOTE: with 4x PGA, a 16.2k internal resistor and our 1nf cap, low pass cutoff is almost 10khz.
 * at 8x 4.2khz. 16x 3.9khz. 2x 29.4khz.
 * bypass PGA and read ADC direct for only the high pass, which is 1.6khz (100k, 1nf)
 *
 * A note on mic placement! The wavelength of sound in air is 343m/s. A 5khz signal is
 * 68.6mm, but thats 360 degrees, at 180 we won't know if it's half a cycle early or late, so
 * effectively we need to be less than 34.3mm apart for 5khz.
 *
 * Another problem happens for low frequencies, like human speech, 100-300hz or so.
 * If we sample too fast we won't fit a whole cycle and correlation goes wonky.
 * More samples takes longer.
 *
 * Slower sample rates have less phase resolution, and take longer to sample of course.
 *
 * Through experimentation, a sample rate of 100KHz with a 512 sample window gives decent results.
 * This gives around +- 32 units of phase detection with mics 100mm apart to about 1700Hz.
 * Above 1700Hz the wavelength is short enough that phsae detection can flip 180 degrees when
 * the source sound is +-90 degrees from the sensor plane.
 * Processing takes about 20ms per axis.
 *
 */


MicData micData1;
MicData micData2;
MicData micData3;
MicData micData4;

AngleFinder angleFinderX;
AngleFinder angleFinderY;

WS2812Driver ws2812Driver;

DisplayVisualizer displayVisualizer;

uint8_t scanMode = 0;

void startSampling() {
	LL_TIM_EnableCounter(TIM3);
}

void stopSampling() {
	LL_TIM_DisableCounter(TIM3);
}

uint32_t micros() {
	return LL_TIM_GetCounter(TIM2);
}


void init() {

	//freeze everything when we debug
	DBGMCU->APB1FZ = 0xffff;
	DBGMCU->APB2FZ = 0xffff;

	initConsoleUart(USART1, DMA1, LL_DMA_CHANNEL_4, LL_DMA_CHANNEL_5);
	ConsoleInit();

	//set DAC to 50%, taking GPA gain into account
	LL_DAC_ConvertData12RightAligned(DAC1, LL_DAC_CHANNEL_1, 2048/16);
	LL_DAC_Enable(DAC1, LL_DAC_CHANNEL_1);
	LL_DAC_TrigSWConversion(DAC1, LL_DAC_CHANNEL_1);

	HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED);
	HAL_ADCEx_Calibration_Start(&hadc2, ADC_SINGLE_ENDED);
	HAL_ADCEx_Calibration_Start(&hadc3, ADC_SINGLE_ENDED);
	HAL_ADCEx_Calibration_Start(&hadc4, ADC_SINGLE_ENDED);


	HAL_OPAMP_SelfCalibrate(&hopamp1);
	HAL_OPAMP_SelfCalibrate(&hopamp2);
	HAL_OPAMP_SelfCalibrate(&hopamp3);
	HAL_OPAMP_SelfCalibrate(&hopamp4);

	HAL_OPAMP_Start(&hopamp1);
	HAL_OPAMP_Start(&hopamp2);
	HAL_OPAMP_Start(&hopamp3);
	HAL_OPAMP_Start(&hopamp4);

	micDataInit(&micData1, &hadc1);
	micDataInit(&micData2, &hadc2);
	micDataInit(&micData3, &hadc3);
	micDataInit(&micData4, &hadc4);

#if	FLIP_X_MICS
	angleFinderInit(&angleFinderX, &micData2, &micData1);
#else
	angleFinderInit(&angleFinderX, &micData1, &micData2);
#endif

#if	FLIP_Y_MICS
	angleFinderInit(&angleFinderY, &micData4, &micData3);
#else
	angleFinderInit(&angleFinderY, &micData3, &micData4);
#endif


	LL_TIM_ClearFlag_UPDATE(TIM3);
//	LL_TIM_EnableIT_UPDATE(TIM3);

//	LL_TIM_ClearFlag_UPDATE(TIM2);
//	LL_TIM_EnableIT_UPDATE(TIM2);

	LL_TIM_EnableCounter(TIM2); //set up as microseconds counter
	LL_TIM_EnableCounter(TIM3);

//	LL_TIM_EnableCounter(TIM4);

	//TOOD calc DC offset on ADC channels. Even with opamp calibration the post-PGA DC can be pretty off
	//for now run micDataProcess a few times, relying on the slow self-adjusting process
	HAL_Delay(5); //let ADCs capture some data
	for (int i = 0; i < 40; i++) {
		micDataSnapshot(&micData1);
		micDataSnapshot(&micData2);
		micDataSnapshot(&micData3);
		micDataSnapshot(&micData4);

		micDataProcess(&micData1);
		micDataProcess(&micData2);
		micDataProcess(&micData3);
		micDataProcess(&micData4);
	}

	ws2812DriverInit(&ws2812Driver, DMA1, LL_DMA_CHANNEL_2, USART3);

	displayVisualizerInit(&displayVisualizer, &angleFinderX, &angleFinderY, &ws2812Driver);

	ConsoleIoSendString("Initialized. Welcome to MicDelta, may I take your order?");
	ConsoleIoSendString(STR_ENDLINE);
	ConsoleIoSendString(CONSOLE_PROMPT);
	fflush(stdout); //necessary if stdout is line buffered
}

void delayUs(uint32_t us) {
	uint32_t tstart = micros();
	while (micros() - tstart < us) {
		//wait
	}
}


void appMain() {
	init();
	while(1) {
		ConsoleProcess();
		displayVisualizerProcess(&displayVisualizer);
	}
}
