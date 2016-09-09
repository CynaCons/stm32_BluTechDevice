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
#include "stm32f4xx_hal.h"

/* USER CODE BEGIN Includes */
#include <string.h>
#include <stdlib.h>
#include "stm32fxxx_hal_BTDevice.h"
#include "stm32f4xx_hal_uart.h"
#include "stm32f4_discovery.h"


#define RX_IT_BUFFER_LENGTH 128
#define SENSOR_DATA_MAX_LENGTH 128
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
TIM_HandleTypeDef htim3;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart6;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
typedef enum{
	DOOR_CLOSED = 0,
	DOOR_OPENED = 1
}DoorState;


uint8_t husart2RxBuffer;
uint8_t husart6RxBuffer;

static uint8_t localDataBuffer[256]; /*!< Buffer to store the data received by the node from the gateway */

static uint16_t localDataLength; /*!< The length of the data that was received */

static volatile uint32_t doorCloseCounter = 0;/*<! Number of times the door was closed */

static volatile uint8_t doorState = DOOR_CLOSED; /*<! State of the door (CLOSED or OPENED) */

static uint8_t deviceCommandReceivedFlag = 0; /*<! Flag to signal that data was received by the node (from the gateway). 0 = nothing, 1 = data received */


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);
static void MX_GPIO_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART6_UART_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
static void initBTDevice();
static uint8_t *getSensorData();
static uint32_t getSensorValue();
static void initReedSwitch();
static void doTheLEDPlay(void *p);
void deviceCommandReceivedCallback(uint8_t *dataBuffer, uint16_t dataLength);
static void deviceCommandReceived(void);

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);


int main(void)
{
	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_ADC1_Init();
	MX_TIM3_Init();
	MX_USART2_UART_Init();
	MX_USART6_UART_Init();

	//Clean the one-byte buffer used to for UART reception
	husart6RxBuffer = 0;
	husart2RxBuffer = 0;

	//Initialize the LEDs
	BSP_LED_Init(LED3);
	BSP_LED_Init(LED4);
	BSP_LED_Init(LED5);
	BSP_LED_Init(LED6);

	BSP_LED_Toggle(LED3);
	BSP_LED_Toggle(LED5);

	//Clear the buffer to remove trash
	memset(localDataBuffer,0, sizeof(localDataBuffer));
	localDataLength = 0;

	//Enable IT upon reception for userUart
	HAL_UART_Receive_IT(&huart6, &husart6RxBuffer,1);

	//Enable IT upon reception for deviceUart
	HAL_UART_Receive_IT(&huart2, &husart2RxBuffer,1);

	//Enable PeriodEllapsed IT for TIM3 (used for periodic data transfer)
	HAL_TIM_Base_Start_IT(&htim3);

	//Init the BTDevice library
	initBTDevice();

	//Init the reed switch sensor: GPIO, NVIC
	initReedSwitch();

	//Display the command menu on userUart
	BTDevice_displayMenu();



	//Automatically initialize the BTDevice with default settings and perfom a signal check
	//The device will try to join the network untill successful. LED visuals will signal success.
	BTDevice_AutoInitTypeDef defaultValues = {0};
	defaultValues.timerPeriodValue = 300;
	defaultValues.autoModeStatus = AUTOMODE_OFF;

	while(BTDevice_initLoop(defaultValues) != BTDevice_OK)
		;

	//Blink the LEDS when the device joins the network
	doTheLEDPlay(NULL);


	/**
	 * Main Loop
	 * the library mainLoop fct will be called to process UART and TIM interupts
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


/***************************************
 * SENSOR PART : ONLY THIS HAS TO CHANGE
 ***************************************/


static uint8_t * getSensorData(void){
	uint32_t value = getSensorValue();
	uint8_t *dataBuffer = malloc(sizeof(uint8_t)*SENSOR_DATA_MAX_LENGTH);
	memset(dataBuffer,0,SENSOR_DATA_MAX_LENGTH);
	sprintf((char *)dataBuffer,"{\"doorCloseCounter\":\"%lu\"}",value);
	return dataBuffer;
}


/**
 * Return the light value
 * @pre CPU temperature sensor must have been initialized before (ADC1 Channel 16)
 */
static uint32_t getSensorValue(void){
	uint32_t tmp = doorCloseCounter;
	doorCloseCounter = 0;
	return tmp;
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
	//The door device is not supposed to receive anything.
	//Just turn on the LEDs
	doTheLEDPlay(NULL);
}


/*
 * This function will make the LEDs blink every 50 ms for 1 seconds
 */
static void doTheLEDPlay(void * p){
	uint32_t startTick = 0, currentTick = 0;
	for(startTick = HAL_GetTick(); ((currentTick = HAL_GetTick() - startTick)) < 2000; currentTick = HAL_GetTick() - startTick){
		if(currentTick % 200 <= 50){
			BSP_LED_On(LED3);
			BSP_LED_Off(LED4);
			BSP_LED_Off(LED5);
			BSP_LED_Off(LED6);
		}else if(currentTick % 200 <= 100 && currentTick % 200 > 50){
			BSP_LED_Off(LED3);
			BSP_LED_On(LED4);
			BSP_LED_Off(LED5);
			BSP_LED_Off(LED6);
		}else if(currentTick % 200 > 100 && currentTick % 200 <= 150){
			BSP_LED_Off(LED3);
			BSP_LED_Off(LED4);
			BSP_LED_On(LED5);
			BSP_LED_Off(LED6);
		}else if(currentTick %200 > 150 && currentTick % 200 <= 200){
			BSP_LED_Off(LED3);
			BSP_LED_Off(LED4);
			BSP_LED_Off(LED5);
			BSP_LED_On(LED6);
		}
	}
	BSP_LED_Off(LED3);
	BSP_LED_Off(LED4);
	BSP_LED_Off(LED5);
	BSP_LED_Off(LED6);
}

/**
 * This function will be called when the reed switch is triggered
 * It will toggle the doorState and send the appropriate message to the Gateway.
 *
 * Need to wait 1000 ticks before toggling again
 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	static uint32_t doorTick = 0;

	if((HAL_GetTick() - doorTick) > 1000 || doorTick == 0){//Wait at least 1000 ticks before inverting doorState (unless it is the first time)
		uint8_t *txBuffer = malloc(sizeof(uint8_t)*SENSOR_DATA_MAX_LENGTH);
		memset(txBuffer,0,SENSOR_DATA_MAX_LENGTH);
		if(GPIO_Pin == GPIO_PIN_15){
			doorCloseCounter++;
			if(doorState == DOOR_OPENED){
				sprintf((char *)txBuffer,"{\"doorState\":\"DOOR_CLOSED\"}");
				BTDevice_sendData(txBuffer, SENSOR_DATA_MAX_LENGTH);
				doorState = DOOR_CLOSED;
			}else{
				sprintf((char *)txBuffer,"{\"doorState\":\"DOOR_OPENED\"}");
				BTDevice_sendData(txBuffer, SENSOR_DATA_MAX_LENGTH);
				doorState = DOOR_OPENED;
			}
		}
	}
	doorTick = HAL_GetTick();
}


/**
 * This function will initialize the reed switch sensor
 */
static void initReedSwitch(){
	//Init PC15 as external interupt !
	GPIO_InitTypeDef GPIO_InitStruct;

	GPIO_InitStruct.Pin = GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;

	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/* Peripheral interrupt init */
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

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

	BTDevice_InitStruct.userHuart = &huart6;
	BTDevice_InitStruct.deviceHuart = &huart2;
	BTDevice_InitStruct.deviceCommandReceivedHandler = &deviceCommandReceivedCallback;
	BTDevice_InitStruct.getSensorDataFunction = &getSensorData;
	BTDevice_InitStruct.signalEventFunction = &doTheLEDPlay;
	BTDevice_init(&BTDevice_InitStruct);
}

/**
 * Every second, this callback will be called following a TIM PeriodEllapsed interruption
 * The following process is executed :
 * 		=>Call BTDevice_timerCallback() to check if new data should be sent (it depends on the time ellapsed
 * 		since last data was sent)
 * 		=>When it's time to send new data, it will call getSensorData (which should return a dynamically allocated
 * 		uint8_buffer * 	containing the data) and then send it through the deviceUart using BTDevice_sendData()
 * @pre TIMx should be configured to trigger an IT every second
 *
 **/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if(htim->Instance == TIM3){
		BTDevice_timerCallback();
	}
}


/**
 * Each time a character is received on either userUart or deviceUart, this callback will be called by the IRQHandler
 * The following process is executed :
 * 		## if the character was received on userUart, BTDevice_userUartCallback will be called
 * 		   the received character is store into the library userInputBuffer
 * 		   Then the userInputBuffer is analyzed by the lirabry to recognize a command. The reception process is
 * 		   restarted in IT mode to enable the next character reception using HAL_UART_Receive_IT
 *
 *		## If the character was received on the deviceUart, BTDevice_deviceUartCallback will be called. This routine
 *		will analyse the received byte, then will enable reception for the rest of the message and will
 *		process it. Finally, once the full message has been received and processed, it will enable the next character
 *		reception using HAL_UART_Receive_IT
 *		(once the message has been fully received, enabling next character reception means receiving the head of the next message
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart->Instance == USART2){
		BTDevice_deviceUartCallback(&husart2RxBuffer);
	}
	if(huart->Instance == USART6){
		BTDevice_userUartCallback(&husart6RxBuffer);
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


/** System Clock Configuration
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
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 84;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 4;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
			|RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV2;
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

/* ADC1 init function */
static void MX_ADC1_Init(void)
{

	ADC_ChannelConfTypeDef sConfig;

	/**Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
	 */
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.ScanConvMode = DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 1;
	hadc1.Init.DMAContinuousRequests = DISABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
	if (HAL_ADC_Init(&hadc1) != HAL_OK)
	{
		Error_Handler();
	}

	/**Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
	 */
	sConfig.Channel = ADC_CHANNEL_0;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}

}

/* TIM3 init function */
static void MX_TIM3_Init(void)
{

	TIM_ClockConfigTypeDef sClockSourceConfig;
	TIM_MasterConfigTypeDef sMasterConfig;

	htim3.Instance = TIM3;
	htim3.Init.Prescaler = 42000;
	htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim3.Init.Period = 1000;
	htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
	if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
	{
		Error_Handler();
	}

	sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
	if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
	{
		Error_Handler();
	}

	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
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

/* USART6 init function */
static void MX_USART6_UART_Init(void)
{
	huart6.Instance = USART6;
	huart6.Init.BaudRate = 115200;
	huart6.Init.WordLength = UART_WORDLENGTH_8B;
	huart6.Init.StopBits = UART_STOPBITS_1;
	huart6.Init.Parity = UART_PARITY_NONE;
	huart6.Init.Mode = UART_MODE_TX_RX;
	huart6.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart6.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart6) != HAL_OK)
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
	__HAL_RCC_GPIOC_CLK_ENABLE();

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

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
