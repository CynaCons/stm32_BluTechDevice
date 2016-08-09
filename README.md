# stm32_BluTechDevice

#What does this lirabry do ?

This repository contains :      
    - the core files of the library : stm32fxx_hal_BTDevice.c and stm32fxxx_hal_BTDevice.h
    - example projects implementing the library for stm32f103 and stm32l152 (for IAR IDE) that can be used out of the box
    - example files to implement the library on it's own (without using the example project)
    - this README.md file

The core .c and .h files must be included to a stm32 project to use and control a BluTec LoRa device via UART.
Examples are provided to build a functionnal program quickly and easily using public functions from the library.

Currently, three boards are supported : stm32f4-disco ; stm32f103 ; stm32l152.

This library is built to be generic and easily reused across all stm32 projects, only the data sensing part has to change.

Check the provided example project (for IAR but the project structure is IDE independent) for copyable code. Only the stm32f103 project is fully commented, check this one first.

# How to get started : 

Clone this repository. 
For STM32F103 : 
    Open the *BluTechDevice_stm32f103_IARexample/EWARM/Project.eww* workspace with IAR.
For STM32L152 :
    Open the *BluTechDevice_stm32l215_IARExample/EWARM/Project.eww* workspace with IAR.
(optionnal) Write the appropriate code to retrieve data from your sensor.
Make sur VLA is enabled in the project settings, otherwise you won't compile. Then configure the target in the project options.
Compile,flash your board and wire the UARTs. 
Reset the board and you should now see the commands menu.


## Complete Guide : 

This lirabry will use two UART :

One, called "deviceUart" will send commands to the BluTech device and receive the command's answer or receive data sent from the REST API

The other, called "userUart", will print a list of commands that can be used to control the BluTech device. It also display the command's result and a lot of other useful things (such as sensor data).	

The second UART is optionnal, only plug it to setup the device and then you can unplug it and move to something else.


Follow the guide below to write the appropriate code to have a functionnal device. Once you can access the application menu : 

=> set the period in seconds between two data transfers : 

	
		set timer period
		[...]
		XX end (where XX is a value in seconds, like 180)
	
=> join the LoRa network :
 
	
		network join
		[...]
		Network join confirmation was received
	
=> enable periodic data transfer according to the period you have set :
	
	
		set automode on
		[...]
		AutoMode Started !
		[...]
		AutoMode ON
		The following data was just sent : YYY (where YYY is the sensor data)
	
=> you can still input commands while automode is ongoing. You can stop automode using :
	
	
		set automode off
		[...]
	
		
	
	


# This is a visual description 
                                                                        The user !
                                                                          XXXXX
                                        => display commands menu          XXXXX
                                        => display values which are sent  XXXXX
    +-------+ sensor data+------------+                                     X       You can plug to the device to input
    |       |          |              | ---------------------------->       X       some settings or check if it's working
    | sensor+---------->              |                                 XXXXXXXXXX
    |       |          |              |                                     X       Once the settings are finished,the
    +-------+          |   STM32Fxxx  |        user uart                    X       devicewill work on it's own.
                       |              |                                     X
                       |              |                                    XXX
                       |              | ----------------------------+    XXX XXX
                       +--------------+                                XXX     XXX
                                            <== perform commands (like
                                      ^     <== network join)
                       +              |     <== input settings (timer period) 
                       |              |     <== send sample data & other
                       |              |
                       |              |
                       |    device    |
                       |    uart      | ^^^ Response following a command
    vvv UART Commands  |              |
       (like networkjoi|              | ^^^ Commands from REST API
       or datatransfer)|              |
                       |              +
                       v

                      +---------------+
                      |               |
                      |               |
                      |     BluTech   |
                      |     device    |
                      |               |
                      |               |
                      +---------------+


# How to make it functionnal ? We have to write some stm32 code.

I highly recommend to use the STM32CubeMX project generator. This lirabry uses the HAL library.
STM32CubeMX will generate the project architecture and initialize all the required peripherals.

STM32CubeMX can be found on ST's website and can generate projects for most IDEs.

To understand how to use, read the example files (main.c, stm32fxxx_it.c) this readme and the example projects

Details for each function can be found in the example files (main.c, stm32fxxx_it.c)

# In main.c 

```

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
            HAL_UART_Transmit(&huart4, "\r\n", sizeof("\r\n"),10°;            
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
```

# In your stm32fxxx_it.c : 


Write the uart IRQHandle for your peripherals

        void USART6_IRQHandler(void){
            HAL_UART_IRQHandler(&huart6);
        }


        void TIM3_IRQHandler(void){
          HAL_TIM_IRQHandler(&htim1);
        }
        
# Once it's all complete, load the program, plug your user uart and start the board. You should see this menu ! 


      "******* Application Menu *********\r\n",
      			"==================================\r\n",
      			"\r\n",
      			"Application Control\r\n",
      			"    menu : display this menu\r\n",
      			"    rs   : to reset the input buffer (type this if you made a typing mistake\r\n",
      			"    get sensor value : to display the current sensor value\r\n",
      			"    set automode on  : to start sending data periodically\r\n",
      			"    set automode off : to start sending data periodically\r\n",
      			"    send data        : to send data to the device just once\r\n",
      			"\r\n",
      			"BluTech Device Control\r\n",
      			"    rf signal check  : perform a signal check\r\n",
      			"    network join     : join the Gateway network\r\n",
      			"    send sample data : to send a sample of data for testing\r\n",
      			"\r\n",
      			"Systick Control\r\n",
      			"    set timer period : to set the value (in seconds) for the periodical timer period\r\n",
      			"    get timer period : to get the current value for periodical timer period\r\n",
      			"\r\n",
      			"WARNING : You have to join a network in order to send data though the RF module \r\n"
