#include "main.h"

#include "arm_math.h"

#include "consoleUart.h"
#include "console.h"

extern ADC_HandleTypeDef hadc1;
extern ADC_HandleTypeDef hadc2;
extern ADC_HandleTypeDef hadc3;
extern ADC_HandleTypeDef hadc4;
extern DMA_HandleTypeDef hdma_adc1;
extern DMA_HandleTypeDef hdma_adc2;
extern DMA_HandleTypeDef hdma_adc3;
extern DMA_HandleTypeDef hdma_adc4;

//extern DAC_HandleTypeDef hdac1;
//
extern OPAMP_HandleTypeDef hopamp1;
extern OPAMP_HandleTypeDef hopamp2;
extern OPAMP_HandleTypeDef hopamp3;
extern OPAMP_HandleTypeDef hopamp4;
//
//extern TIM_HandleTypeDef htim4;
//extern DMA_HandleTypeDef hdma_tim4_up;

//extern UART_HandleTypeDef huart1;

/*
 * a snap has a big pulse about 2ms, with trailing signal. about 400hz, 2khz, 20khz.
 * with mics 260-300mm apart, get a .7ms delay
 * sample at about 50khz, will get 5.12ms in 256 samples, 2.56ms in 128
 * NOTE: with 4x PGA, a 16.2k internal resistor and our 1nf cap, low pass cutoff is almost 10khz.
 * at 8x 4.2khz. 16x 3.9khz. 2x 29.4khz.
 * bypass PGA and read ADC direct for only the high pass, which is 1.6khz (100k, 1nf)
 */

int16_t cbuf[4][ADC_BUF_SIZE];


void init() {

	//freeze everything when we debug
	DBGMCU->APB1FZ = 0xffff;
	DBGMCU->APB2FZ = 0xffff;

	initConsoleUart(USART1, DMA1, LL_DMA_CHANNEL_4, LL_DMA_CHANNEL_5);

	LL_DAC_ConvertData12RightAligned(DAC1, LL_DAC_CHANNEL_1, 2048/4);
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

	HAL_ADC_Start_DMA(&hadc1, (uint32_t *) &cbuf[0][0], ADC_BUF_SIZE);
	HAL_ADC_Start_DMA(&hadc2, (uint32_t *) &cbuf[1][0], ADC_BUF_SIZE);
	HAL_ADC_Start_DMA(&hadc3, (uint32_t *) &cbuf[2][0], ADC_BUF_SIZE);
	HAL_ADC_Start_DMA(&hadc4, (uint32_t *) &cbuf[3][0], ADC_BUF_SIZE);


	LL_TIM_ClearFlag_UPDATE(TIM3);
	LL_TIM_EnableIT_UPDATE(TIM3);

	LL_TIM_ClearFlag_UPDATE(TIM2);
	LL_TIM_EnableIT_UPDATE(TIM2);

	LL_TIM_EnableCounter(TIM3);
	LL_TIM_EnableCounter(TIM4);

	//TOOD calc DC offset on ADC channels




}


void appMain() {

	init();

	while(1) {
		ConsoleProcess();
	}
}
