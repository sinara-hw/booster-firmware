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

#define ADC_CDR_ADDRESS 	0x40012308
#define NCHANNELS			6
#define NSAMPLES			100
#define BUFFER_SIZE			NCHANNELS * NSAMPLES
static uint16_t 			adc_samples[BUFFER_SIZE] = { 0 };
static uint16_t 			converted_values[BUFFER_SIZE] = { 0 };
static double				averaged_values[NCHANNELS] = { 0 };
static double				averaged_voltage[NCHANNELS] = { 0 };

static void adc_gpio_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOF, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOC, ENABLE);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOC, &GPIO_InitStructure);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
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
	TIM_TimeBaseStructure.TIM_Prescaler = ((SystemCoreClock) / 100) - 1; // 1 ms
	TIM_TimeBaseStructure.TIM_Period = 10 - 1;
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
	ADC_RegularChannelConfig(ADC1, ADC_Channel_13, 1, ADC_SampleTime_15Cycles); // PC1
	ADC_RegularChannelConfig(ADC1, ADC_Channel_12, 2, ADC_SampleTime_15Cycles); // PC2

	ADC_RegularChannelConfig(ADC3, ADC_Channel_14, 1, ADC_SampleTime_15Cycles); // PF4
	ADC_RegularChannelConfig(ADC3, ADC_Channel_15, 2, ADC_SampleTime_15Cycles); // PF5
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
	ADC_InitStructure.ADC_NbrOfConversion = 2;

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

		/* Turn LED3 off: Half Transfer */
		GPIO_ToggleBits(GPIOC, GPIO_Pin_8);
		// Add code here to process first half of buffer (ping)

		memcpy(converted_values, adc_samples, BUFFER_SIZE / 2);
	}

	/* Test on DMA Stream Transfer Complete interrupt */
	if (DMA_GetITStatus(DMA2_Stream0, DMA_IT_TCIF0))
	{
		GPIO_SetBits(GPIOC, GPIO_Pin_10);
		/* Clear DMA Stream Transfer Complete interrupt pending bit */
		DMA_ClearITPendingBit(DMA2_Stream0, DMA_IT_TCIF0);

		/* Turn LED3 on: End of Transfer */
		GPIO_ToggleBits(GPIOC, GPIO_Pin_9);

		memcpy(converted_values + BUFFER_SIZE / 2, adc_samples + BUFFER_SIZE / 2, BUFFER_SIZE / 2);

		/* ADC1_CH1 ADC2_CH2 ADC3_CH3
		 * Skipping channel 2 values
		 */

		uint8_t samples = 0;
		memset(averaged_values, 0, NCHANNELS);
		for (int i = 0; i < NSAMPLES; i+= 6, samples++)
		{
			averaged_values[0] += converted_values[i];
			averaged_values[2] += converted_values[i + 2];
			averaged_values[3] += converted_values[i + 3];
			averaged_values[5] += converted_values[i + 5];
		}
		samples++;

		averaged_values[0] /= samples;
		averaged_values[2] /= samples;
		averaged_values[3] /= samples;
		averaged_values[5] /= samples;

		averaged_voltage[0] = ((averaged_values[0] * 2.50) / 4096);
		averaged_voltage[2] = ((averaged_values[2] * 2.50) / 4096);
		averaged_voltage[3] = ((averaged_values[3] * 2.50) / 4096);
		averaged_voltage[5] = ((averaged_values[5] * 2.50) / 4096);

		for (int j = 0; j < NCHANNELS; j++) printf("%d ", converted_values[j]);
		printf("\n");

		printf("values %.2f %.2f %.2f %.2f", averaged_values[0], averaged_values[2], averaged_values[3], averaged_values[5]);
		printf("\n");
		printf("voltage %.2f %.2f %.2f %.2f", averaged_voltage[0], averaged_voltage[2], averaged_voltage[3], averaged_voltage[5]);
		printf("\n\n");

		GPIO_ResetBits(GPIOC, GPIO_Pin_10);
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
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, DISABLE);
	ADC_TempSensorVrefintCmd(DISABLE);

	return (uint16_t) (avaraged / 100);
}

