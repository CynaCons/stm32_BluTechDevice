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

/*********************
	 * Includes & defines
	 *********************/
	#include "stm32f1xx_hal.h"
	#include "stm32fxxx_hal_BTDevice.h" //BTDevice driver header file
	#include "stm32f1xx_hal_uart.h" //HAL UART header file
	
	#include <string.h>
	#include <stdlib.h>
	
	
	#define SENSOR_DATA_MAX_LENGTH 128
	
	/********************
	 * Private variables
	 ********************/
	//Peripheral handlers
	ADC_HandleTypeDef hadc1;
	TIM_HandleTypeDef htim1;
	TIM_HandleTypeDef htim3;
	UART_HandleTypeDef huart4;
	UART_HandleTypeDef huart1;
	
	//Use this buffer to store data received from user uart
	uint8_t rxITBuffer[256];
	
	//Input buffer index. 
	uint16_t rxITIndex = 0;
	
	//Single byte buffer to store characters temporarily from the userHuart
	uint8_t huart4RxBuffer = 0;
	
	//Single byte buffer to store characters temporarily from the deviceUart
	uint8_t huart1RxBuffer = 0;
	
	
	#define NB_MENU_LINES 21 /*!< Number of lines to display in the menu */
	
	
	/*******************************
	 * Private functions prototypes
	 *******************************/
	//STM32CubeMX-generated functions
	static void MX_GPIO_Init(void);
	static void MX_ADC1_Init(void);
	static void MX_TIM1_Init(void);
	static void MX_TIM3_Init(void);
	static void MX_UART4_Init(void);
	static void MX_USART1_UART_Init(void);
	
	void SystemClock_Config(void);
	void Error_Handler(void);
	
	static void initBTDevice();
	
	static void storeNewValueIntoInputBuffer(void);
	static void resetInputBuffer(void);
	static void deviceCommandReceivedCallback(uint8_t *dataBuffer, uint16_t dataLength);
	
	static float getCPUTemperature(void);
	static uint8_t *getSensorData(void);
	
	
	
	int main(void)
	{
	
	
	
	    /* MCU Configuration----------------------------------------------------------*/
	
	    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
	    HAL_Init();
	
	    /* Configure the system clock */
	    SystemClock_Config();
	
	    /* Initialize all configured peripherals */
	    
	    MX_GPIO_Init();
	    MX_ADC1_Init();
	    MX_TIM1_Init();
	    MX_TIM3_Init();
	    MX_UART4_Init();
	    MX_USART1_UART_Init();
	
	    memset(rxITBuffer,0,sizeof(rxITBuffer)); //fill the input buffer with \0
	
	    HAL_UART_Receive_IT(&huart4, &huart4RxBuffer, 1);
	
	    //This is to receive data from the BluTech device
	    HAL_UART_Receive_IT(&huart1,&huart1RxBuffer,1);
	
	    //Start timer for periodical data transfer
	    HAL_TIM_Base_Start_IT(&htim3);
	
	    //Start the driver
	    initBTDevice();
	
	    //Display the commands menu
	    BTDevice_displayMenu();
	
	
	    /* Infinite loop */
	    while (1)
	    {
	
	    }
	
	}
	
	/*****************************************************************************
	 * The following section deals with peripherals interrupts callback functions
	 ****************************************************************************/
	
	
	/**
	 * TIMER Callback function. 
	 * Periodically, this function is called and data is sent.
	 * Period value can be changed through uart interface (userHuart)
	 */
	void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	    //First, check which timer period has ellapsed
	    if(htim->Instance == TIM3){ 
	        if(BTDevice_timerCallback()){ //Is it time to send the next data ?
	            //If yes, which mean we have waited the required amout of seconds
	            //Then send the data !
	            uint8_t *tmp = getSensorData();
	            BTDevice_sendData(tmp,SENSOR_DATA_MAX_LENGTH);
	            free(tmp);
	        }
	    }
	}   
	
	
	/**
	 * UART Reception Callback (this is called once a reception interupt has been acknowledged
	 */
	void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	    //Make sure it's the right uart
	    if(huart->Instance == USART1){
	        //When deviceHuart (wired to the BTDevice) receive a message, the first byte is analysed to determine the message's nature.
	        //Once the first byte of the message is analysed, the body will be received and analysed inside the driver.
	        //The result of this analysis (confirmation, failure, data received) will be displayed with userUart
	        BTDevice_deviceUartCallback(&huart1RxBuffer);
	    }
	    if(huart->Instance == UART4){
	        //When the userHuart (wired to the computre) receive a character, the following actions are perfomed 
	        storeNewValueIntoInputBuffer(); //Store newly received character into the rxITBuffer
	        BTDevice_readInputBuffer(); //Parse the rxITBuffer to recognize commands
	        HAL_UART_Receive_IT(&huart4,&huart4RxBuffer,1); //Enable receiving ot the new character
	    }
	}
	
	
        
        /**************************************************************************************
         * The following section deals with command/data reception (from the gateway) callback
         **************************************************************************************/
       

	/**
	 * This function is called when the BTDevice receives data from the Gateway
	 */
         static void deviceCommandReceivedCallback(uint8_t *dataBuffer, uint16_t dataLength){
            //Display the data or do some special action like turning on a LED or actionner
            //In this case we simply print the received data over the userUart
            uint8_t txBuffer[128];
            memset(txBuffer, 0, sizeof(txBuffer));
            sprintf((char *)txBuffer, "The following data was received from the gateway :\r\n");
            HAL_UART_Transmit(&huart4, txBuffer, sizeof(txBuffer), 10);
            HAL_UART_Transmit(&huart4, dataBuffer, dataLength, 10);
            HAL_UART_Transmit(&huart4, "\r\n", sizeof("\r\n"),10);            
        }


	/********************
	 * Private functions
	 ********************/
	
	
	/**
	 * Call this function once to start the BTDevice driver
	 */
	static void initBTDevice(void){
	    BTDevice_InitTypeDef BTDevice_InitStruct;
	    BTDevice_InitStruct.userHuart = &huart4;
	    BTDevice_InitStruct.deviceHuart = &huart1;
	    BTDevice_InitStruct.userInputBuffer = rxITBuffer;
	    BTDevice_InitStruct.resetInputBufferHandler = &resetInputBuffer;
	    BTDevice_InitStruct.deviceCommandReceivedHandler = &deviceCommandReceivedCallback;
	    BTDevice_init(&BTDevice_InitStruct);
	}
	
	
	/**
	 * Once a character is received from UART (userHuart), stores it into a larger reception buffer
	 */
	static void storeNewValueIntoInputBuffer(void) {
	    rxITBuffer[rxITIndex] = huart4RxBuffer;
	    rxITIndex++;
	}
	
	
	/**
	 * Reset the inputBuffer
	 */
	static void resetInputBuffer(void) {
	    memset(rxITBuffer, 0, sizeof(rxITBuffer));
	    rxITIndex = 0;
	}
	
	

	/*********************************************************************************
	 * The following section acquire the CPU Temperature (THE CONVERSION IS WRONG !)
	 *
	 * This section only should be modified and adapted to match the data sensor
	 *********************************************************************************/
	
	/**
	 * Returns a string (uint8_t array) formated to contain the CPU temperature in the JSON format
	 **/
	static uint8_t *getSensorData(){
	    uint8_t *dataBuffer = malloc(sizeof(uint8_t)*SENSOR_DATA_MAX_LENGTH);
	    memset(dataBuffer,0,sizeof(dataBuffer));
	    float tempValue = getCPUTemperature();
	    sprintf((char *)dataBuffer,"{\"CPU_temperature(conversion is wrong)\":\"%lu\"}",(uint32_t)tempValue);
	    return dataBuffer;
	}
	
	
	/**
	 * Return the CPU temperature converted in celsius degrees
	 * @pre CPU temperature sensor must have been initialized before (ADC1 Channel 16)
	 */
	static float getCPUTemperature(void){
	    uint32_t sensorValue;
	    float convertedValue;
	    HAL_ADC_Start(&hadc1);
	    HAL_ADC_PollForConversion(&hadc1,1000);
	    sensorValue=HAL_ADC_GetValue(&hadc1);
	    convertedValue = sensorValue *3000.0f; //Vref is 3000mV
	    convertedValue /= 0xfff; //12 bit resolution
	    convertedValue /= 1000.0f; //mV to V
	    convertedValue -= 0.7f;
	    convertedValue /= .0034f;
	    convertedValue += 25.0f; //Add 25°C
	    HAL_ADC_Stop(&hadc1);
	    return convertedValue;
	}


/*******************************************************************
 * The following section in automatically generated by STM32CubeMX
 *******************************************************************/


/**
 * System Clock Configuration
 */
void SystemClock_Config(void)
{

    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_PeriphCLKInitTypeDef PeriphClkInit;

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = 16;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
    RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
        |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }

    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
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

    /**Common config 
    */
    hadc1.Instance = ADC1;
    hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    if (HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }

    /**Configure Regular Channel 
    */
    sConfig.Channel = ADC_CHANNEL_TEMPSENSOR;
    sConfig.Rank = 1;
    sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler();
    }

}

/* TIM1 init function */
static void MX_TIM1_Init(void)
{

    TIM_ClockConfigTypeDef sClockSourceConfig;
    TIM_MasterConfigTypeDef sMasterConfig;

    htim1.Instance = TIM1;
    htim1.Init.Prescaler = 64000;
    htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
    htim1.Init.Period = 1000;
    htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    htim1.Init.RepetitionCounter = 0;
    if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
    {
        Error_Handler();
    }

    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }

    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
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
    htim3.Init.Prescaler = 64000;
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

/* UART4 init function */
static void MX_UART4_Init(void)
{

    huart4.Instance = UART4;
    huart4.Init.BaudRate = 115200;
    huart4.Init.WordLength = UART_WORDLENGTH_8B;
    huart4.Init.StopBits = UART_STOPBITS_1;
    huart4.Init.Parity = UART_PARITY_NONE;
    huart4.Init.Mode = UART_MODE_TX_RX;
    huart4.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart4.Init.OverSampling = UART_OVERSAMPLING_16;
    if (HAL_UART_Init(&huart4) != HAL_OK)
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
