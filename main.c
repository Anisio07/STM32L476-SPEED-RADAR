/**
 ******************************************************************************
 * @file    main.c
 * @author  Anisio  Gomes
 * @version V1.0
 * @date    08-December-2017
 * @brief   Default main function.
 ******************************************************************************
 */


#include "stm32l4xx.h"
#include "stm32l476g_discovery.h"
#include "stm32l4xx_hal_gpio_ex.h"
#include "stm32l4xx_hal_gpio.h"
#include "stm32l4xx_ll_usart.h"
#include "stm32l476g_discovery_glass_lcd.h"
#include "clock.h"
#include <string.h>
#include "arm_math.h"
#include "stdlib.h"
#include <math.h>

/* FFT settings */
#define SAMPLES                    512             /* 256 real party and 256 imaginary parts */
#define FFT_SIZE                SAMPLES / 2        /* FFT size is always the same size as we have samples, so 256 in our case */

#define FFT_BAR_MAX_HEIGHT        120             /* 120 px on the LCD */

/* Global variables */
float32_t Input[SAMPLES];
float32_t Output[FFT_SIZE];

#define TEST_LENGTH_SAMPLES 2048



/* -------------------------------------------------------------------

* External Input and Output buffer Declarations for FFT Bin Example

* ------------------------------------------------------------------- */

extern float32_t testInput_f32_10khz[TEST_LENGTH_SAMPLES];

static float32_t testOutput[TEST_LENGTH_SAMPLES/2];



/* ------------------------------------------------------------------

* Global variables for FFT Bin Example

* ------------------------------------------------------------------- */

uint32_t fftSize = 1024;

uint32_t ifftFlag = 0;

uint32_t doBitReverse = 1;

/* Reference index at which max energy of bin ocuurs */

uint32_t refIndex = 213, testIndex = 0;

ADC_ChannelConfTypeDef ADCchannel;
ADC_HandleTypeDef ADChandle;
GPIO_InitTypeDef GPIOconf;
TIM_HandleTypeDef Timer4Handle;
UART_HandleTypeDef huart2;

// UART Ports:
// ===================================================
// PA.0 = UART4_TX (AF8)   |  PA.1 = UART4_RX (AF8)
// PB.6 = USART1_TX (AF7)  |  PB.7 = USART1_RX (AF7)
// PD.5 = USART2_TX (AF7)  |  PD.6 = USART2_RX (AF7)

void GPIO_Init(void)
{

    GPIO_InitTypeDef GPIO_InitStruct;

    __HAL_RCC_GPIOD_CLK_ENABLE();
    // Peripheral clock enable
   __HAL_RCC_USART2_CLK_ENABLE();
   /**USART2 GPIO Configuration
       PA2     ------> USART2_TX
       PA3     ------> USART2_RX
       */
       GPIO_InitStruct.Pin = GPIO_PIN_5|GPIO_PIN_6;
       GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
       GPIO_InitStruct.Pull = GPIO_NOPULL;
       GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
       GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
       HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

}

void HuartConf(void)
{
	//uint16_t
	/*if (HAL_UART_DeInit(&huart2) != HAL_OK)
		{
			return HAL_ERROR;
		}
*/
       huart2.Instance = USART2;
       huart2.Init.BaudRate = 9600;
       huart2.Init.WordLength = UART_WORDLENGTH_8B; // sending 8-Bit ascii chars
       huart2.Init.StopBits = UART_STOPBITS_1;
       huart2.Init.Parity = UART_PARITY_NONE;
       huart2.Init.Mode = UART_MODE_TX_RX;
       huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
       huart2.Init.OverSampling = UART_OVERSAMPLING_8;
       huart2.Init.OneBitSampling = UART_ONEBIT_SAMPLING_DISABLED ;
       huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
       HAL_UART_Init(&huart2);


     /*  if (HAL_UART_Init(&huart2) != HAL_OK)
       	{
       		/ Initialization Error /
       		return HAL_ERROR;
       	}

       	return HAL_OK;
       	*/
}

void init(void)
{
	SystemClock_Config(); // Switch System Clock = 80 MHz
	HAL_Init();
	GPIO_Init();
	HuartConf();
	//HAL_UART_Init(&huart2);
	BSP_LCD_GLASS_Init();

	__HAL_RCC_GPIOB_CLK_ENABLE(); // Enable MISO pin
	__HAL_RCC_GPIOE_CLK_ENABLE(); // Enable MISO pin
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_USART2_CLK_ENABLE();
	__HAL_RCC_ADC_CLK_ENABLE();


		GPIOconf.Pin=GPIO_PIN_2;
		GPIOconf.Mode=GPIO_MODE_ANALOG_ADC_CONTROL; // get the signal from the generator
		GPIOconf.Pull=GPIO_NOPULL;
		GPIOconf.Alternate=0;
		HAL_GPIO_Init(GPIOA, &GPIOconf);


	//Configuration for GPIO pin PB2, output LED#

		//GPIOconf.Mode = GPIO_MODE_OUTPUT_PP;
		GPIOconf.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

	HAL_GPIO_Init(GPIOB, &GPIOconf);

	//Configuration for GPIO pin PE8, output LED
	GPIOconf.Pin =  GPIO_PIN_8;
	HAL_GPIO_Init(GPIOE, &GPIOconf);

	Timer4Handle.Instance = TIM4;
	Timer4Handle.Init.CounterMode = TIM_COUNTERMODE_UP;
	Timer4Handle.Init.ClockDivision = 0;
	Timer4Handle.Init.Prescaler = 60000;
	Timer4Handle.Init.Period = 800;

	__HAL_RCC_TIM4_CLK_ENABLE();
	HAL_TIM_Base_Init(&Timer4Handle);
	HAL_TIM_Base_Start_IT(&Timer4Handle);

	HAL_NVIC_SetPriority(TIM4_IRQn, 7, 0); // middle priority
	HAL_NVIC_EnableIRQ(TIM4_IRQn);

	HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, 1 );


}

void TIM4_IRQHandler()
{
	HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_8);
	__HAL_TIM_CLEAR_FLAG(&Timer4Handle, TIM_FLAG_UPDATE);
}

void ADCset()
{
	ADCchannel.Channel = ADC_CHANNEL_7;
	ADCchannel.Offset = 0;
	ADCchannel.Rank = ADC_REGULAR_RANK_1;
	ADCchannel.SamplingTime = ADC_SAMPLETIME_2CYCLE_5;

	ADChandle.Instance = ADC1;
	ADChandle.Init.ClockPrescaler = ADC_CLOCKPRESCALER_PCLK_DIV2;
	ADChandle.Init.Resolution = ADC_RESOLUTION_12B;
	ADChandle.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	ADChandle.Init.ScanConvMode = DISABLE;
	ADChandle.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	ADChandle.Init.ContinuousConvMode = DISABLE;
	ADChandle.Init.DiscontinuousConvMode = DISABLE;
	ADChandle.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	ADChandle.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;

	HAL_ADC_Init(&ADChandle);
	HAL_ADC_ConfigChannel(&ADChandle, &ADCchannel);
	ADC_Enable(&ADChandle);
}


void bspv()
{
	HAL_Init();
	BSP_LED_Init(LED_RED);
	BSP_LED_Init(LED_GREEN);

	while(1)
	{
		BSP_LED_On(LED_RED);
		HAL_Delay(100);
		BSP_LED_On(LED_GREEN);
		HAL_Delay(100);
		BSP_LED_Off(LED_RED);
		HAL_Delay(100);
		BSP_LED_Off(LED_GREEN);

	}
}


void gpiov()
{
	init();
	while(1)
	{
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, 1 );
		HAL_Delay(100);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, 1 );
		HAL_Delay(100);
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, 0 );
		HAL_Delay(100);
		HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, 0 );
	}
}


void timerv()
{

	init();
	while(1)
	{
	}
}

void adc()

{
		init();
		ADCset();
		char vol[10];
		uint32_t value;
		int len;
		while(1)
		{
			HAL_ADC_Start(&ADChandle);
			while(HAL_ADC_PollForConversion(&ADChandle, 1000)!=HAL_OK);
			value = HAL_ADC_GetValue(&ADChandle);

			HAL_ADC_Stop(&ADChandle);

			len=sprintf(vol, "%3lu\n", value);
			HAL_UART_Transmit(&huart2, (uint8_t *)vol, len, 100);
			//HAL_UART_Transmit(&huart2, (uint8_t *)" \n", len, 1);
			BSP_LCD_GLASS_DisplayString((uint32_t*)vol);

			//HAL_Delay(200);
			BSP_LCD_GLASS_Clear();

		}
}

int32_t fft(void)
{
	arm_status status;

	arm_cfft_radix4_instance_f32 S;

	float32_t maxValue;


	status = ARM_MATH_SUCCESS;



	/* Initialize the CFFT/CIFFT module */

	status = arm_cfft_radix4_init_f32(&S, fftSize,

	  								ifftFlag, doBitReverse);



	/* Process the data through the CFFT/CIFFT module */

	arm_cfft_radix4_f32(&S, testInput_f32_10khz);





	/* Process the data through the Complex Magnitude Module for

	calculating the magnitude at each bin */

	arm_cmplx_mag_f32(testInput_f32_10khz, testOutput,

	  				fftSize);



	/* Calculates maxValue and returns corresponding BIN value */

	arm_max_f32(testOutput, fftSize, &maxValue, &testIndex);



	if(testIndex !=  refIndex)

	{

		status = ARM_MATH_TEST_FAILURE;

	}



	/* ----------------------------------------------------------------------

	** Loop here if the signals fail the PASS check.

	** This denotes a test failure

	** ------------------------------------------------------------------- */



	if( status != ARM_MATH_SUCCESS)

	{

		while(1);

	}



    while(1);                             /* main function does not return */
}
int main()
{
	//bspv();
	//gpiov();
	//timerv();
	//comwc();
	//adc();
	 fft();

}
