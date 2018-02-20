/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32l4xx.h"
#include "stm32l476g_discovery.h"
#include "stm32l4xx_hal_def.h"


TIM_HandleTypeDef MYTIMER;
COMP_HandleTypeDef comp333;
COMP_InitTypeDef comp233;
USART_InitTypeDef myusart;
USART_HandleTypeDef usarthandle;


int counter = 0;
int n = 5;

// the interrupt function; display speed on LED
void TIM4_IRQHandler(void)
{
	char vol[5];
	sprintf(vol, "% 6d", counter/2);
	BSP_LCD_GLASS_DisplayString(vol);

	int first_digit = 0;
	int second_digit = 0;
	first_digit = counter/10;
	second_digit = counter%10;
	int first_4bits[4] = {0,0,0,0};
	int second_4bits[4] = {0,0,0,0};
	int i1 = 0;
	int i2 = 0;
	while (first_digit != 0)
	{
		int rem = first_digit%2;
		first_digit = first_digit/2;
		first_4bits[i1] = rem;
		i1++;
	}

	while (second_digit != 0)
		{
		    int left = second_digit%2;
			second_digit = second_digit/2;
			second_4bits[i2] = left;
			i2++;
		}

	int transmit[8];
	int i = 0;
	for (i=0;i<4;i++)
	{
		transmit[i] = first_4bits[3-i];
	}
	for (i=4;i<8;i++)
	{
		transmit[i] = second_4bits[7-i];
	}
	HAL_USART_Transmit(&usarthandle, transmit, 8, 100);

	counter = 0;

	__HAL_TIM_CLEAR_FLAG(&MYTIMER, TIM_FLAG_UPDATE);
}

// Initialization functions
void Init(void)
{
	HAL_Init();
	__HAL_RCC_TIM4_CLK_ENABLE(); // enable clock
	BSP_LCD_GLASS_Init(); // enable LED
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_USART2_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();


	// usart initialization
	myusart.BaudRate = 57600;
	myusart.WordLength = USART_WORDLENGTH_9B;
	myusart.StopBits = USART_STOPBITS_1;
	myusart.Parity = USART_PARITY_EVEN;
	myusart.Mode = USART_MODE_TX_RX;

	usarthandle.Instance = USART2;
	usarthandle.Init = myusart;
	HAL_USART_Init(&usarthandle);

    // timer initialization; timer interrupt occur every 1 second
	MYTIMER.Instance = TIM4;
	MYTIMER.Init.CounterMode = TIM_COUNTERMODE_UP;
	MYTIMER.Init.ClockDivision = 0;
	MYTIMER.Init.Prescaler = 5000;
	MYTIMER.Init.Period = 800;
	HAL_TIM_Base_Init(&MYTIMER);

    // the interrupt initialization
	HAL_NVIC_EnableIRQ(TIM4_IRQn);
	HAL_NVIC_SetPriority(TIM4_IRQn, 7, 0);

    // comparator initialization
	comp233.WindowMode = COMP_WINDOWMODE_DISABLE;
	comp233.Mode = COMP_POWERMODE_MEDIUMSPEED;
	comp233.NonInvertingInput = COMP_INPUT_PLUS_IO2;
	comp233.InvertingInput = COMP_INPUT_MINUS_1_2VREFINT;
	comp233.Hysteresis = COMP_HYSTERESIS_NONE;
	comp233.OutputPol = COMP_OUTPUTPOL_NONINVERTED;
	comp233.BlankingSrce = COMP_BLANKINGSRC_NONE;
	comp233.TriggerMode = COMP_TRIGGERMODE_NONE;

	comp333.Instance = COMP1;
	comp333.Init = comp233;
	HAL_COMP_Init(&comp333);


    // configuration for GPIO pin PB2; output LED
	GPIO_InitTypeDef PINC;
		PINC.Pin = GPIO_PIN_2;
		PINC.Mode = GPIO_MODE_ANALOG;
		PINC.Pull = GPIO_NOPULL;
		PINC.Speed = GPIO_SPEED_FREQ_MEDIUM;
		PINC.Alternate = 0;
		HAL_GPIO_Init(GPIOB, &PINC);

	// configuration for USART
		GPIO_InitTypeDef GPIO_InitStruct;
		GPIO_InitStruct.Pin = GPIO_PIN_2|GPIO_PIN_3;
		GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
		GPIO_InitStruct.Pull = GPIO_NOPULL;
		GPIO_InitStruct.Speed = GPIO_SPEED_LOW;
		GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);


}

void self_test(void)
{
	HAL_Init();
	BSP_LED_Init(LED_RED);
	BSP_LED_Init(LED_GREEN);

	if (HAL_USART_GetState(&usarthandle)==0x01U && HAL_USART_GetError(&usarthandle)==0x00U)
	{
		BSP_LED_On(LED_GREEN);

	}
	else
	{
		switch (HAL_USART_GetError(&usarthandle))
		{
		case ¡®0x01U¡¯:
		BSP_LCD_GLASS_DisplayString('Parity error');
		BSP_LED_On(LED_RED);
		break;
		case ¡®0x02U¡¯:
		BSP_LCD_GLASS_DisplayString('Noise error');
		BSP_LED_On(LED_RED);
		break;
		case ¡®0x04U¡¯:
		BSP_LCD_GLASS_DisplayString('frame error');
		BSP_LED_On(LED_RED);
		break;
		case ¡®0x08U¡¯:
		BSP_LCD_GLASS_DisplayString('Overrun error');
		BSP_LED_On(LED_RED);
		break;
		default:
		BSP_LCD_GLASS_DisplayString('unknown error');
		BSP_LED_On(LED_RED);
		}
	}


}

int main(void)
{
	Init();
	self_test();

	int current_value=0, old_value=0;

	HAL_COMP_Start(&comp333); // start comparator
	HAL_TIM_Base_Start_IT(&MYTIMER); // start timer


	counter = 0;
	current_value = HAL_COMP_GetOutputLevel(&comp333);
	old_value = current_value;

      while (1)
      {
    	  current_value = HAL_COMP_GetOutputLevel(&comp333);

    	  if (old_value!=current_value)
    	  {
    		  counter = counter+1;
    	  }
    	  old_value=HAL_COMP_GetOutputLevel(&comp333);

      }

}
