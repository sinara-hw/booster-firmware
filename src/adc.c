/*
 * adc.c
 *
 *  Created on: Sep 28, 2017
 *      Author: wizath
 */

#include "adc.h"
#include "stm32f4xx_adc.h"
#include "stm32f4xx_gpio.h"
#include "stm32f4xx_dma.h"
#include "channels.h"

#define ADC_CDR_ADDRESS 	0x40012308
#define NCHANNELS			24
#define NSAMPLES			50
#define BUFFER_SIZE			NCHANNELS * NSAMPLES

static uint16_t 			adc_samples[BUFFER_SIZE] = { 0 };
static uint16_t 			converted_values[BUFFER_SIZE] = { 0 };
static double				averaged_values[NCHANNELS] = { 0 };
static double				averaged_voltage[NCHANNELS] = { 0 };

extern channel_t channels[8];

static void adc_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	// ADC IN 0-3
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	// ADC IN 10-13
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	// ADC IN 4-9, 14-15
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOF, &GPIO_InitStructure);
}

void adc_timer_init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	/* Time base configuration */
	TIM_TimeBaseStructInit(&TIM_TimeBaseStructure);
	TIM_TimeBaseStructure.TIM_Prescaler = ((SystemCoreClock) / 10) - 1; // 1 ms
	TIM_TimeBaseStructure.TIM_Period = 1;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	/* TIM2 TRGO selection
	 * ADC_ExternalTrigConv_T2_TRGO
	 */
	TIM_SelectOutputTrigger(TIM2, TIM_TRGOSource_Update);

	/* TIM2 enable counter */
	TIM_Cmd(TIM2, ENABLE);
}


static void adc_channel_init(void)
{
	// ADC 1

	// rf channel 0
	ADC_RegularChannelConfig(ADC1, ADC_Channel_0, 1, ADC_SampleTime_3Cycles); // PA0
	ADC_RegularChannelConfig(ADC1, ADC_Channel_1, 2, ADC_SampleTime_3Cycles); // PA1

	// rf channel 1
	ADC_RegularChannelConfig(ADC1, ADC_Channel_2, 3, ADC_SampleTime_3Cycles); // PA2
	ADC_RegularChannelConfig(ADC1, ADC_Channel_3, 4, ADC_SampleTime_3Cycles); // PA3

	// rf channel 5
	ADC_RegularChannelConfig(ADC1, ADC_Channel_10, 5, ADC_SampleTime_3Cycles); // PC0
	ADC_RegularChannelConfig(ADC1, ADC_Channel_11, 6, ADC_SampleTime_3Cycles); // PC1

	// rf channel 6
	ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 7, ADC_SampleTime_3Cycles); // PC2
	ADC_RegularChannelConfig(ADC1, ADC_Channel_13, 8, ADC_SampleTime_3Cycles); // PC3

	// ADC3

	// rf channel 2
	ADC_RegularChannelConfig(ADC3, ADC_Channel_4, 1, ADC_SampleTime_3Cycles); // PF6
	ADC_RegularChannelConfig(ADC3, ADC_Channel_5, 2, ADC_SampleTime_3Cycles); // PF7

	// rf channel 3
	ADC_RegularChannelConfig(ADC3, ADC_Channel_6, 3, ADC_SampleTime_3Cycles); // PF8
	ADC_RegularChannelConfig(ADC3, ADC_Channel_7, 4, ADC_SampleTime_3Cycles); // PF9

	// rf channel 4
	ADC_RegularChannelConfig(ADC3, ADC_Channel_8, 5, ADC_SampleTime_3Cycles); // PF10
	ADC_RegularChannelConfig(ADC3, ADC_Channel_9, 6, ADC_SampleTime_3Cycles); // PF3

	// rf channel 7
	ADC_RegularChannelConfig(ADC3, ADC_Channel_14, 7, ADC_SampleTime_3Cycles); // PF4
	ADC_RegularChannelConfig(ADC3, ADC_Channel_15, 8, ADC_SampleTime_3Cycles); // PF5
}


static void adc_dma_init(void)
{
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_DMA2, ENABLE);
	DMA_InitTypeDef DMA_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	DMA_InitStructure.DMA_Channel = DMA_Channel_0;
	DMA_InitStructure.DMA_Memory0BaseAddr = (uint32_t) &adc_samples;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t) ADC_CDR_ADDRESS; /* CDR_ADDRESS; Packed ADC1, ADC2, ADC3 */
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralToMemory;
	DMA_InitStructure.DMA_BufferSize = BUFFER_SIZE; /* Count of 16-bit words */
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;
	DMA_InitStructure.DMA_FIFOMode = DMA_FIFOMode_Enable;
	DMA_InitStructure.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStructure.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStructure.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(DMA2_Stream0, &DMA_InitStructure);

	/* Enable DMA Stream Half / Transfer Complete interrupt */
	DMA_ITConfig(DMA2_Stream0, DMA_IT_TC | DMA_IT_HT, ENABLE);

	/* DMA2_Stream0 enable */
	DMA_Cmd(DMA2_Stream0, ENABLE);

	/* Enable the DMA Stream IRQ Channel */
	NVIC_InitStructure.NVIC_IRQChannel = DMA2_Stream0_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}


void adc_init(void)
{
	ADC_CommonInitTypeDef 	ADC_CommonInitStructure;
	ADC_InitTypeDef 		ADC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC3, ENABLE);

	adc_gpio_init();
	adc_dma_init();
	adc_timer_init();

	/* ADC Common Init */
	ADC_CommonInitStructure.ADC_Mode = ADC_TripleMode_RegSimult;
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_1; /* 3 half-words one by one, 1 then 2 then 3 */
	ADC_CommonInitStructure.ADC_TwoSamplingDelay =  ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = ENABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE; /* Conversions Triggered */
	ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_Rising;
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T2_TRGO;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 16;

	ADC_Init(ADC1, &ADC_InitStructure);
	ADC_Init(ADC3, &ADC_InitStructure); /* Mirror on ADC3 */

	/* ADC1&3 Pair can only be used in triple mode
	   So soft disable ADC2 measurements */
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_NbrOfConversion = 0;
	ADC_Init(ADC2, &ADC_InitStructure);

	adc_channel_init();

	/* Enable DMA request after last transfer (Multi-ADC mode)  */
	ADC_MultiModeDMARequestAfterLastTransferCmd(ENABLE);

	/* Enable ADC1 */
	ADC_Cmd(ADC1, ENABLE);

	/* Enable ADC2 */
	ADC_Cmd(ADC2, ENABLE);

	/* Enable ADC3 */
	ADC_Cmd(ADC3, ENABLE);
}


void DMA2_Stream0_IRQHandler(void)
{
	/* Test on DMA Stream Half Transfer interrupt */
	if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_HTIF0))
	{
		/* Clear DMA Stream Half Transfer interrupt pending bit */
		DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_HTIF0);
		memcpy(converted_values, adc_samples, BUFFER_SIZE / 2);
	}

	/* Test on DMA Stream Transfer Complete interrupt */
	if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_TCIF0))
	{
		/* Clear DMA Stream Transfer Complete interrupt pending bit */
		DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);

		/* Turn LED3 on: End of Transfer */
		GPIO_ToggleBits(BOARD_LED2);

		memcpy(converted_values + BUFFER_SIZE / 2, adc_samples + BUFFER_SIZE / 2, BUFFER_SIZE / 2);
//
//		/* ADC1_CH1 ADC2_CH2 ADC3_CH3
//		 * Skipping channel 2 values
//		 */
//
		uint8_t samples = 0;
		memset(averaged_values, 0, NCHANNELS);
		for (int i = 0; i < NSAMPLES; i+= NCHANNELS, samples++)
		{
			// rf channel 0
			averaged_values[0] += converted_values[i];
			averaged_values[1] += converted_values[i + 3];

			// rf channel 1
			averaged_values[2] += converted_values[i + 6];
			averaged_values[3] += converted_values[i + 9];

			// rf channel 2
			averaged_values[4] += converted_values[i + 2];
			averaged_values[5] += converted_values[i + 5];

			// rf channel 3
			averaged_values[6] += converted_values[i + 8];
			averaged_values[7] += converted_values[i + 11];

			// rf channel 4
			averaged_values[8] += converted_values[i + 14];
			averaged_values[9] += converted_values[i + 17];

			// rf channel 5
			averaged_values[10] += converted_values[i + 12];
			averaged_values[11] += converted_values[i + 15];

			// rf channel 6
			averaged_values[12] += converted_values[i + 18];
			averaged_values[13] += converted_values[i + 21];

			// rf channel 7
			averaged_values[14] += converted_values[i + 20];
			averaged_values[15] += converted_values[i + 23];
		}

		samples++;

		for (int i = 0; i < 16; i++) averaged_values[i] /= samples;
		for (int i = 0; i < 16; i++) averaged_voltage[i] = ((averaged_values[i] * 2.50) / 4096);

		channels[0].adc_ch1 = averaged_values[0];
		channels[0].adc_ch2 = averaged_values[1];

		channels[1].adc_ch1 = averaged_values[2];
		channels[1].adc_ch2 = averaged_values[3];

		channels[2].adc_ch1 = averaged_values[4];
		channels[2].adc_ch2 = averaged_values[5];

		channels[3].adc_ch1 = averaged_values[6];
		channels[3].adc_ch2 = averaged_values[7];

		channels[4].adc_ch1 = averaged_values[8];
		channels[4].adc_ch2 = averaged_values[9];

		channels[5].adc_ch1 = averaged_values[10];
		channels[5].adc_ch2 = averaged_values[11];

		channels[6].adc_ch1 = averaged_values[12];
		channels[6].adc_ch2 = averaged_values[13];

		channels[7].adc_ch1 = averaged_values[14];
		channels[7].adc_ch2 = averaged_values[15];
	}
}


uint16_t adc_autotest(void)
{
	/* Test wether ADC Vref is working properly */
	ADC_InitTypeDef       	ADC_InitStructure;
	ADC_CommonInitTypeDef 	ADC_CommonInitStructure;

	/* IMPORTANT: populates structures with reset values */
	ADC_StructInit(&ADC_InitStructure);
	ADC_CommonStructInit(&ADC_CommonInitStructure);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

	/* init ADCs in independent mode, div clock by two */
	ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
	ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div2;
	ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
	ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
	ADC_CommonInit(&ADC_CommonInitStructure);

	/* init ADC1: 12bit, single-conversion */
	ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
	ADC_InitStructure.ADC_ExternalTrigConvEdge = 0;
	ADC_InitStructure.ADC_ExternalTrigConv = 0;
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitStructure.ADC_NbrOfConversion = 1;
	ADC_Init(ADC1, &ADC_InitStructure);

	/* Enable VREF_INT & Temperature channel */
	ADC_TempSensorVrefintCmd(ENABLE);

	/* Enable ADC1 **************************************************************/
	ADC_Cmd(ADC1, ENABLE);

	ADC_RegularChannelConfig(ADC1, ADC_Channel_17, 1, ADC_SampleTime_480Cycles);

	int avaraged = 0;
	uint16_t value = 0;

	// 1.18 -- 1.21 -- 1.24V
	// ADC values 1933 - 1982 - 2031
	for (int i = 0; i < 100; i++)
	{
		ADC_SoftwareStartConv(ADC1);
		while ((ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET));
		value = ADC_GetConversionValue(ADC1);
		avaraged += value;

		if (value <= 1933 || value >= 2031)
			return 0;
	}

	ADC_DeInit();
	ADC_Cmd(ADC1, DISABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);
	ADC_TempSensorVrefintCmd(DISABLE);

	return (uint16_t) (avaraged / 100);
}

uint8_t adc_get_data(double * arr)
{
	memcpy(arr, averaged_values, sizeof(double) * NCHANNELS);
	return NCHANNELS;
}
