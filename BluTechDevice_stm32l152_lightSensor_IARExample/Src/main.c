/**
 ******************************************************************************
 * File Name          : main.c
 * Description        : Main program body
 ******************************************************************************
 *
 * COPYRIGHT(c) 2016 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */
/* Includes ------------------------------------------------------------------*/
#include "stm32l1xx_hal.h"

/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdlib.h>

#include "stm32l1xx_hal_uart.h"
#include "stm32fxxx_hal_BTDevice.h"
#include "stm32l152c_discovery.h"
#include "buzzer.h"


#define RX_IT_BUFFER_LENGTH 128
#define SENSOR_DATA_MAX_LENGTH 128


/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim4;
UART_HandleTypeDef huart1; //Handler structure for userUart
UART_HandleTypeDef huart2; //Handler structure for deviceUart


/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
uint8_t husart1RxBuffer;
uint8_t husart2RxBuffer;

static uint8_t localDataBuffer[256]; /*!< Buffer to store the data received by the node from the gateway */

static uint16_t localDataLength; /*!< The length of the data that was received */

static uint8_t deviceCommandReceivedFlag = 0; /*<! Flag to signal that data was received by the node (from the gateway). 0 = nothing, 1 = data received */


/* USER CODE END PV */
/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);
static void MX_GPIO_Init(void);
static void MX_ADC_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
static void deviceCommandReceivedCallback(uint8_t *dataBuffer, uint16_t dataLength);
static void deviceCommandReceived();
static void initBTDevice();
static uint32_t getSensorValue();
static uint8_t *getSensorData(void);
static void doTheLEDPlay();
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);


int main(void)
{
	/* MCU Configuration----------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_ADC_Init();
	MX_TIM2_Init();
	MX_TIM4_Init(); //Used to generate PWM frequency to stimulate the passive buzzer
	MX_USART1_UART_Init(); //Init userHuart
	MX_USART2_UART_Init(); //Init deviceHuart

	//Initialize the LEDs
	BSP_LED_Init(LED_GREEN);
	BSP_LED_Init(LED_BLUE);
	BSP_LED_Toggle(LED_BLUE);

	//Clear the buffer to remove trash
	memset(localDataBuffer,0, sizeof(localDataBuffer));
	localDataLength = 0;

	//Enable IT upon reception for userUart
	HAL_UART_Receive_IT(&huart1, &husart1RxBuffer,1);
	//Enable IT upon reception for deviceUart
	HAL_UART_Receive_IT(&huart2,&husart2RxBuffer,1);

	//Enable IT for TIM2 (used for periodic data transfer)
	HAL_TIM_Base_Start_IT(&htim2);

	//Init the BTDevice library
	initBTDevice();

	//Display the command menu on userUart
	BTDevice_displayMenu();

	//Automatically initialize the BTDevice with default settings and perfom a signal check
	//The device will try to join the network untill successful. LED visuals will signal success.
	BTDevice_AutoInitTypeDef defaultValues;
	defaultValues.timerPeriodValue = 300;
	defaultValues.autoModeStatus = AUTOMODE_ON;
	while(BTDevice_initLoop(defaultValues) != BTDevice_OK)
		;

	doTheLEDPlay();

	/**
	 * Main Loop
	 *
	 * the library mainLoop fct will be called to process UART and TIM interupts
	 *
	 * when a message was received by the device (from the gateway), a special handler function is called
	 */
	while (1)
	{
		BTDevice_mainLoop();

		//True when data is received from the gateway
		if(deviceCommandReceivedFlag){
			deviceCommandReceivedFlag = 0; //Lower the flag
			deviceCommandReceived(); //Call the handler to analyse the received data.
		}
	}
}




/**************************************************************************************************************
 * PWM_IT CALLBACKS PART : READ COMMENTS FOR CULTURE, I DONT USE PWM_IT ANYMORE TO MAKE THE BUZZER FUNCTIONNAL
 **************************************************************************************************************/

/**
 * To enable this callback, TIMx CHANNELx must be started in PWM_IT mode.
 *
 * This callback will be executed when the PWM pulse is done (I dont know when it's the case, either when TIMx->CNT = TIMx->CCRx or = TIMx->ARR)
 * Here is the right place to change the TIMx->CCRx value
 *
 * (TIMx->ARR/TIMx->CCRx = pwm duty cycle. Change either ARR or CCRx registers to change duty cycle)
 * Then PWM pulse will be set to high while TIMx->CNT is lower (or equal ?) to TIMx->CCRx. When it's greater, the pulse will be set to low
 * When TIMx->CNT is equals to TIMx->ARR, TIM->CNT is set to 0 thus repeating the previous sequence.
 */
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim){
	if(htim->Instance == TIM4){
		//			TIM4->CCR3 = (uint32_t)5000/(notes[level]*2); //Set new value for the pwm duty cycle
	}
}


/**
 * To enable this callback, TIMx CHANNELx must be started in PWM_IT mode
 *
 * This callback will be called when TIMx->CNT reach TIMx->ARR.
 * Here is the right place to change the TIMx->ARR value and then reload it generating a realod event with TIMx->EGR = 1;
 *
 * ARR stands for AutoReloadRegister. Basically thats the period value in TIMER ticks (coreFrequency/TIMx_prescalerValue)
 * When TIMx->CNT (CNT = count) is equal to ARR, then CNT is set to 0 and will count again from 0 to ARR.
 */
void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim){
	if(htim->Instance == TIM4){

	}
}


/***************************************
 * SENSOR PART : ONLY THIS HAS TO CHANGE
 ***************************************/


/**
 * Return a JSON formatted dynamically allocated(with malloc) string (= uint8_t *buffer) containing the
 * value from @getSensorValue
 * This allocated buffer is then free'd (call to 'free' in the library) once it's data has been sent.
 */
static uint8_t * getSensorData(void){
	uint32_t value = getSensorValue();
	uint8_t *dataBuffer = malloc(sizeof(uint8_t)*SENSOR_DATA_MAX_LENGTH);
	memset(dataBuffer,0,SENSOR_DATA_MAX_LENGTH);
	sprintf((char *)dataBuffer,"{\"brightness\":\"%lu\"}",value);
	return dataBuffer;
}


/**
 * Return the light sensor's value
 */
static uint32_t getSensorValue(void){
	uint32_t sensorValue;
	HAL_ADC_Start(&hadc);
	HAL_ADC_PollForConversion(&hadc,1000);
	sensorValue=HAL_ADC_GetValue(&hadc);
	HAL_ADC_Stop(&hadc);
	return sensorValue;
}


/**
 * This callback is called from the library whenever a command/data was received by the
 * BTDevice (sent by the Gateway which means following a REST API post)
 *
 * It will copy the data received into a local buffer and raise a flag so that the data will be analyzed next time CPU is "free"
 *
 * @pre this function's address must be provided to the library during the BTDevice_Init();
 * @pre the localDataBuffer must be clear of trash (recommend to use memset to fill with zeroes)
 */
void deviceCommandReceivedCallback(uint8_t *dataBuffer, uint16_t dataLength){
	strcpy((char *)localDataBuffer,(char *)dataBuffer); //Get a copy of the data we received
	localDataLength = dataLength;
	deviceCommandReceivedFlag = 1; //Raise the flag. See main loop
}


/*
 * This function will be called from Main Loop when data/command has been received from the gateway.
 */
void deviceCommandReceived(){
	//Start the TIM4 on channel 3 (pin PB8) to generate PWM
	HAL_TIM_PWM_Start(&htim4,TIM_CHANNEL_3);

	//Display the received data on userUart
	uint8_t txBuffer[128];
	memset(txBuffer, 0 ,sizeof(txBuffer));
	sprintf((char *)txBuffer,"The following data was received by the device :\r\n");
	HAL_UART_Transmit(&huart1, txBuffer, sizeof(txBuffer),10);
	HAL_UART_Transmit(&huart1, localDataBuffer, localDataLength,10);
	HAL_UART_Transmit(&huart1, (uint8_t *)"\r\n",sizeof("\r\n"),10);


	//Analyze the received data
	if(strcmp((const char *)localDataBuffer,"DoorAlert")){ //Door has been opened for too long

		//Make some flashy moves with the LEDs to signal data/command was received
		doTheLEDPlay();

		//Play the Imperial March melody with the buzzer
		Buzzer_playImperialMarch();

		//Signal the end of the melody with flashy moves
		doTheLEDPlay();

	}else if(strcmp((const char *)localDataBuffer,"FireAlert")){ //Temperature is too high
		doTheLEDPlay();
		Buzzer_playImperialMarch();
		doTheLEDPlay();
	}

	//No more PWM to generate, stop the TIM4 Channel 3
	HAL_TIM_PWM_Stop(&htim4,TIM_CHANNEL_3);
}


/*
 * This function will make the LEDs blink every 50 ms for 0.5 seconds
 */
static void doTheLEDPlay(){
	uint32_t startTick = 0, currentTick = 0;
	for(startTick = HAL_GetTick(); ((currentTick = HAL_GetTick() - startTick)) < 500; currentTick = HAL_GetTick() - startTick){
		if(currentTick % 100 <= 50){
			BSP_LED_On(LED_GREEN);
			BSP_LED_Off(LED_BLUE);
		}else{
			BSP_LED_Off(LED_GREEN);
			BSP_LED_On(LED_BLUE);
		}
	}
	BSP_LED_Off(LED_GREEN);
	BSP_LED_Off(LED_BLUE);
}


/******************************************************************************
 * INIT & LIBRARY PART: THIS SHOULD NOT BE CHANGED UNLESS YOU KNOW WHAT YOU DO
 ******************************************************************************/

/**
 * Initialize the library :
 * 		=> set the UART peripherals to be used : userUart and deviceUart
 * 		=> set the buffer's address to be used to analyze received characters from user input : userInputBuffer
 * 		=> set the user input buffer reset routine : resetInputBufferHandler
 * 		=> set the function to handle commands/data received by the node from the gateway : deviceCommandReceivedFunction
 * 		=> set the function to perform the data sensing and formatting (JSON format) : getSensorDataFunction
 * 		=> set the function to be called when the sensor's data has been sent : dataSentCallback
 * 		=> set the maximum length of the data payload : sensorDataMaxLength
 */
static void initBTDevice(void){

	BTDevice_InitTypeDef BTDevice_InitStruct = {0};

	BTDevice_InitStruct.userHuart = &huart1;
	BTDevice_InitStruct.deviceHuart = &huart2;
	BTDevice_InitStruct.deviceCommandReceivedHandler = &deviceCommandReceivedCallback;
	BTDevice_InitStruct.getSensorDataFunction = &getSensorData;
	BTDevice_InitStruct.dataSentCallback = &doTheLEDPlay;
	BTDevice_InitStruct.sensorDataMaxLength = SENSOR_DATA_MAX_LENGTH;
	BTDevice_init(&BTDevice_InitStruct);
}


/**
 * Every second, this callback will be executed following a TIM PeriodEllapsed interruption
 * Read the library comments for complete details on the process.
 *
 * @brief Check if enough time since last transfer has been waited. If so, send the data, if not, wait. Time between two transfer can be changed
 * through commands
 * @pre TIMx should be configured to trigger an IT every second
 *
 **/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if(htim->Instance == TIM2){
		BTDevice_timerCallback();
	}
}

/**
 * Each time a character is received on either userUart or deviceUart, this callback will be called by the IRQHandler
 *
 * The following process is executed :
 *
 * 		## if the character was received on userUart, the character is store into the user input buffer.
 * 		Then the library's userInputBuffer is analyzed to recognize a command. Finally, Reception for next character is enabled
 *
 *		## If the character was received on the deviceUart, BTDevice_deviceUartCallback will be called. This routine
 *		will analyse the byte received, then will enable reception for the rest of the message and will
 *		process it. Finally, once the full message has been received and processed, it will enable reception for the next message
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart->Instance == USART2){
		BTDevice_deviceUartCallback(&husart2RxBuffer);
	}
	if(huart->Instance == USART1){
		BTDevice_userUartCallback(&husart1RxBuffer);
	}
}

/*******************************************************************************
 * SYSTEM CLOCK Init and PERIPHERALS Init AUTOMATICALLY GENERATED BY STM32CUBEMX
 *
 * Some of these init functions will then call the appropriate MSP funtion.
 * See stm32l1xx_hal_msp.c
 *
 * THIS SHOULD NOT BE MODIFIED UNLESS YOU KNOW WHAT YOU ARE DOING
 *******************************************************************************/



/*
 * System Clock Configuration
 */
void SystemClock_Config(void)
{

	RCC_OscInitTypeDef RCC_OscInitStruct;
	RCC_ClkInitTypeDef RCC_ClkInitStruct;

	__HAL_RCC_PWR_CLK_ENABLE();

	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = 16;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL4;
	RCC_OscInitStruct.PLL.PLLDIV = RCC_PLL_DIV2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
	{
		Error_Handler();
	}

	HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

	HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

	/* SysTick_IRQn interrupt configuration */
	HAL_NVIC_SetPriority(SysTick_IRQn, 0, 0);
}

/* ADC init function */
static void MX_ADC_Init(void)
{

	ADC_ChannelConfTypeDef sConfig;

	/**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
	 */
	hadc.Instance = ADC1;
	hadc.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
	hadc.Init.Resolution = ADC_RESOLUTION_12B;
	hadc.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc.Init.ScanConvMode = ADC_SCAN_DISABLE;
	hadc.Init.EOCSelection = ADC_EOC_SEQ_CONV;
	hadc.Init.LowPowerAutoWait = ADC_AUTOWAIT_DISABLE;
	hadc.Init.LowPowerAutoPowerOff = ADC_AUTOPOWEROFF_DISABLE;
	hadc.Init.ChannelsBank = ADC_CHANNELS_BANK_A;
	hadc.Init.ContinuousConvMode = DISABLE;
	hadc.Init.NbrOfConversion = 1;
	hadc.Init.DiscontinuousConvMode = DISABLE;
	hadc.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc.Init.DMAContinuousRequests = DISABLE;
	if (HAL_ADC_Init(&hadc) != HAL_OK)
	{
		Error_Handler();
	}

	/**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
	 */
	sConfig.Channel = ADC_CHANNEL_0;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_4CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}

}

/* TIM2 init function */
static void MX_TIM2_Init(void)
{

	TIM_ClockConfigTypeDef sClockSourceConfig;
	TIM_MasterConfigTypeDef sMasterConfig;

	htim2.Instance = TIM2;
	htim2.Init.Prescaler = 32000;
	htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim2.Init.Period = 1000;
	htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
	{
		Error_Handler();
	}

	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}

	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}

}
/* TIM4 init function
 *
 * So far i'm only using Channel3. Other channel are not fully configured
 * */
static void MX_TIM4_Init(void)
{

	TIM_ClockConfigTypeDef sClockSourceConfig;
	TIM_MasterConfigTypeDef sMasterConfig;
	TIM_OC_InitTypeDef sConfigOC;

	htim4.Instance = TIM4;
	htim4.Init.Prescaler = 3200;
	htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim4.Init.Period = 10000;
	htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
	{
		Error_Handler();
	}

	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}

	if (HAL_TIM_PWM_Init(&htim4) != HAL_OK)
	{
		Error_Handler();
	}

	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}

	sConfigOC.OCMode = TIM_OCMODE_PWM1;
	sConfigOC.Pulse = 25;
	sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
	sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
	if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
	{
		Error_Handler();
	}

	sConfigOC.Pulse = 25;
	if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
	{
		Error_Handler();
	}

	sConfigOC.Pulse = 5000;
	if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
	{
		Error_Handler();
	}

	sConfigOC.Pulse = 750;
	if (HAL_TIM_PWM_ConfigChannel(&htim4, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
	{
		Error_Handler();
	}

	HAL_TIM_MspPostInit(&htim4);

}

/* USART1 init function */
static void MX_USART1_UART_Init(void)
{

	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK)
	{
		Error_Handler();
	}

}

/* USART2 init function */
static void MX_USART2_UART_Init(void)
{

	huart2.Instance = USART2;
	huart2.Init.BaudRate = 115200;
	huart2.Init.WordLength = UART_WORDLENGTH_8B;
	huart2.Init.StopBits = UART_STOPBITS_1;
	huart2.Init.Parity = UART_PARITY_NONE;
	huart2.Init.Mode = UART_MODE_TX_RX;
	huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart2.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart2) != HAL_OK)
	{
		Error_Handler();
	}

}

/** Pinout Configuration
 */
static void MX_GPIO_Init(void)
{

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

}

/******************************************************************************
 * ERROR HANDLING PART
 *
 * Error will be analyzed here and the appropriate action will be performed
 ******************************************************************************/

/**
 * @brief  This function is executed in case of error occurrence.
 * @param  None
 * @retval None
 */
void Error_Handler(void)
{
	/* USER CODE BEGIN Error_Handler */
	/* User can add his own implementation to report the HAL error return state */
	while(1)
	{
	}
	/* USER CODE END Error_Handler */
}

#ifdef USE_FULL_ASSERT

/**
 * @brief Reports the name of the source file and the source line number
 * where the assert_param error has occurred.
 * @param file: pointer to the source file name
 * @param line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
    ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */

}

#endif

/**
 * @}
 */

/**
 * @}
 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
