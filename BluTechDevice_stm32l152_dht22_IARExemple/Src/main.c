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
#include <stdlib.h>
#include <string.h>
#include "stm32l1xx_hal_uart.h"
#include "stm32fxxx_hal_BTDevice.h"
#include "stm32l152c_discovery.h"
#include "dht22.h"

/* USER CODE BEGIN Includes */
#define RX_IT_BUFFER_LENGTH 128
#define SENSOR_DATA_MAX_LENGTH 128
/* USER CODE END Includes */

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim4;
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DHT22_HandleTypeDef dht;

/* USER CODE BEGIN PV */
/* Private variables ---------------------------------------------------------*/
uint8_t husart1RxBuffer;
uint8_t husart2RxBuffer;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void Error_Handler(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM4_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);

/* USER CODE BEGIN PFP */
/* Private function prototypes -----------------------------------------------*/
static void deviceCommandReceivedCallback(uint8_t *dataBuffer, uint16_t dataLength);
static void deviceCommandReceivedHandler();
static void initBTDevice();
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
static uint8_t *getSensorData(void);
static uint8_t *getTemperatureData(void);
static uint8_t *getHumidityData(void);

static void initDHT22(void);
static void doTheLEDPlay();

/* USER CODE BEGIN 0 */
static uint8_t localDataBuffer[256];
static uint16_t localDataLength;
static uint8_t deviceCommandReceivedFlag = 0;

/* USER CODE END 0 */

int main(void)
{

	/* USER CODE BEGIN 1 */

	/* USER CODE END 1 */

	/* MCU Configuration----------------------------------------------------------*/

	/* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	HAL_Init();

	/* Configure the system clock */
	SystemClock_Config();

	/* Initialize all configured peripherals */
	MX_GPIO_Init();
	MX_TIM2_Init();
	MX_TIM4_Init();
	MX_USART1_UART_Init();
	MX_USART2_UART_Init();

	//Initialize the LEDs
	BSP_LED_Init(LED_GREEN);
	BSP_LED_Init(LED_BLUE);
	BSP_LED_Toggle(LED_BLUE);

	initDHT22();
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

	while (1)
	{

		BTDevice_mainLoop();

		//When data/command is received from the gateway, deviceCommandReceivedFlag is raised
		if(deviceCommandReceivedFlag){
			deviceCommandReceivedFlag = 0;
			deviceCommandReceivedHandler();
		}
	}
}


static uint8_t *getSensorData(void)
{
	DHT22_ReadData(&dht);
	uint8_t *dataBuffer = malloc(sizeof(uint8_t)*SENSOR_DATA_MAX_LENGTH);
	uint8_t *tmpTemp = getTemperatureData();
	uint8_t *tmpHumid = getHumidityData();
	sprintf((char *)dataBuffer,"{\"temperature\":\"%s\",\"humidity\":\"%s\"}",tmpTemp,tmpHumid);
	free(tmpTemp);
	free(tmpHumid);
	return dataBuffer;
}

/**
 * Return a JSON formatted dynamically allocated(with malloc) string (= uint8_t *buffer) containing the
 * value from @getSensorValue
 * Dont forget to the buffer's address as soon as it's not needed anymore (currently in TIM Callback)
 */
static uint8_t *getTemperatureData(){
	uint8_t *dataBuffer = malloc(sizeof(uint8_t)*SENSOR_DATA_MAX_LENGTH);
	memset(dataBuffer,0,SENSOR_DATA_MAX_LENGTH);
	uint32_t value = (uint32_t)dht.temp;
	sprintf((char *)dataBuffer,"%lu",value);
	return dataBuffer;
}

static uint8_t *getHumidityData(){
	uint8_t *dataBuffer = malloc(sizeof(uint8_t)*SENSOR_DATA_MAX_LENGTH);
	memset(dataBuffer,0, SENSOR_DATA_MAX_LENGTH);
	uint32_t value = (uint32_t)dht.hum;
	sprintf((char *)dataBuffer,"%lu",value);
	return dataBuffer;
}



/**
 * This callback is called from the library whenever a command/data was received by the BTDevice (sent by the Gateway thus following a REST API post)
 *
 * This function's address must be provided to the library during the BTDevice_Init();
 *
 */
void deviceCommandReceivedCallback(uint8_t *dataBuffer, uint16_t dataLength){
	//Here we can simply display the data on the userHuart or do some special action
	strcpy((char *)localDataBuffer,(char *)dataBuffer); //Get a copy of the data we received
	localDataLength = dataLength;
	deviceCommandReceivedFlag = 1; //Raise the flag. See main loop
}

/*
 * This handler will be called when data/command has been received from the gateway
 *
 */
//TODO PREVENT SPAM IN THE LIBRARY
void deviceCommandReceivedHandler(){
	//Display the received data on userUart
	uint8_t txBuffer[128];
	memset(txBuffer, 0 ,sizeof(txBuffer));
	sprintf((char *)txBuffer,"The following data was received by the device :\r\n");
	HAL_UART_Transmit(&huart1, txBuffer, sizeof(txBuffer),10);
	HAL_UART_Transmit(&huart1, localDataBuffer, localDataLength,10);
	HAL_UART_Transmit(&huart1, (uint8_t *)"\r\n",sizeof("\r\n"),10);

	if(strcmp((const char *)localDataBuffer,"Hello World!")){
		//Make some flashy moves with the LEDs to signal data/command was received
		doTheLEDPlay();

		//Play the melody

		//Signal the end of the melody with flashy moves
		doTheLEDPlay();
	}else if(strcmp((const char *)localDataBuffer,"FireAlert")){
		doTheLEDPlay();
		doTheLEDPlay();
	}
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


void initDHT22() {
	/* USER CODE BEGIN 2 */
	dht.gpioPin = GPIO_PIN_8;
	dht.gpioPort = GPIOB;
	dht.timHandle.Instance = TIM4;
	dht.timChannel = TIM_CHANNEL_3;
	dht.timerIRQn = TIM4_IRQn;
	dht.gpioAlternateFunction = GPIO_AF2_TIM4;
	if( DHT22_Init(&dht) == DHT22_OK){

	}
}

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef* htim){
	DHT22_InterruptHandler(&dht);
}


/******************************************************************************
 * INIT & LIBRARY PART: THIS SHOULD NOT BE CHANGED UNLESS YOU KNOW WHAT YOU DO
 ******************************************************************************/

/**
 * Initialize the library :
 * 		=> set the UART peripherals to be used : userUart and deviceUart
 * 		=> set the buffer's address to be used to analyze received characters from user input : userInputBuffer
 * 		=> set the user input buffer reset routine : resetInputBufferHandler
 * 		=> set the routine to handle commands/data received by the node from the gateway
 */
static void initBTDevice(void){

	BTDevice_InitTypeDef BTDevice_InitStruct;
	BTDevice_InitStruct.userHuart = &huart1;
	BTDevice_InitStruct.deviceHuart = &huart2;
	BTDevice_InitStruct.deviceCommandReceivedHandler = &deviceCommandReceivedCallback;
	BTDevice_InitStruct.getSensorDataFunction = &getSensorData;
	BTDevice_InitStruct.dataSentCallback = &doTheLEDPlay;
	BTDevice_InitStruct.sensorDataMaxLength = SENSOR_DATA_MAX_LENGTH;
	BTDevice_init(&BTDevice_InitStruct);
}


/**
 * Every second, this callback will be called following a TIM PeriodEllapsed interruption
 * The following process is executed :
 * 		=>Call BTDevice_timerCallback() to check if new data should be sent (it depends on the time ellapsed
 * 		since last data was sent)
 * 		=>When it's time to send new data, it will call getSensorData (which should return a dynamically allocated
 * 		uint8_buffer * 		containing the data) and then send it through the deviceUart using BTDevice_sendData()
 * 		=>rise a flag to perform some LED visuals to signal the user that data is beeing sent
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
 * The following process is executed :
 * 		## if the character was received on userUart, the character is store into the user input buffer by calling
 * 		storeNewValueIntoInputBuffer()
 * 		=>Then the userInputBuffer is analyzed by the lirabry to recognize a command. The reception process is
 * 		restarting in IT mode to enable the next character reception using HAL_UART_Receive_IT
 *
 *		## If the character was received on the deviceUart, BTDevice_deviceUartCallback will be called. This routine
 *		will send analyse the byte received, then will enable reception for the rest of the message and will
 *		process it. Finally, once the full message has been received and processed, it will enable the next character
 *		reception using HAL_UART_Receive_IT
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart->Instance == USART2){
		BTDevice_deviceUartCallback(&husart2RxBuffer);
	}

	if(huart->Instance == USART1){
		BTDevice_userUartCallback(&husart1RxBuffer);
	}
}

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

	htim4.Instance = TIM4;
	htim4.Init.Prescaler = 32000;
	htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim4.Init.Period = 1000;
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


	sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
	sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
	if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
	{
		Error_Handler();
	}


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
