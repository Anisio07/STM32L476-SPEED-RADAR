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
			
TIM_HandleTypeDef MYTIMER;
COMP_HandleTypeDef comp333;
COMP_InitTypeDef comp233;

int counter = 0;
int n = 5;

// the interrupt function; display speed on LED
void TIM4_IRQHandler(void)
{
	char vol[5];
	sprintf(vol, "% 6d", counter/2);
	BSP_LCD_GLASS_DisplayString(vol);
	counter = 0;

	__HAL_TIM_CLEAR_FLAG(&MYTIMER, TIM_FLAG_UPDATE);
}

// Initialization functions
void Init(void)
{
	HAL_Init();
	__HAL_RCC_TIM4_CLK_ENABLE(); // enable clock
	BSP_LCD_GLASS_Init(); // enable LED

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


}

int main(void)
{
	int current_value=0, old_value=0;
	Init();

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
