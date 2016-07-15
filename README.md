# stm32_BluTechDevice
Drivers to command the BluTech devices using an STM32 device

                                                                        The user !
                                                                          XXXXX
                                          +display commands menu          XXXXX
                                          +display values which are sent  XXXXX
    +-------+ sensor data+------------+                                     X       You can plug to the device to input
    |       |          |              | ---------------------------->       X       some settings or check if it's working
    | sensor+---------->              |                                 XXXXXXXXXX
    |       |          |              |                                     X       Once the settings are finished,the
    +-------+          |   STM32Fxxx  |        user uart                    X       devicewill work on it's own.
                       |              |                                     X
                       |              |                                    XXX
                       |              | ----------------------------+    XXX XXX
                       +--------------+                                XXX     XXX
                                              +perform commands (like
                                      ^        network join)
                       +              |       +input settings (timer period) 
                       |              |        +send sample data & other
                       |              |
                       |              |
                       |    device    |
                       |    uart      | +Response following a command
       UART Commands   |              |
       (like networkjoi|              | +Commands from REST API
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


How to use ? 


I highly recommend to use an STM32CubeMX generated project with the HAL library.

You will find details for each function the example files (main.c, stm32fxxx_it.c)


# In your main.c : 


	
	//Be careful, this file only give you functions examples. This is not a full main.c !
	
	//Include the BTDevice header file. Replace first x !
	#include "stm32fxxx_hal_BTDevice.h" 
	
	//Include the uart related functions. Replace first x !
	#include "stm32fxxx_hal_uart.h"
	
	#include <string.h> //For string manipulation
	#include <stdlib.h> //For malloc (freeing is done in the drivers)
	
	//Lets say our sensor data won't be more than 128 bytes
	#define SENSOR_DATA_MAX_LENGTH 128
	
	
	//Use buffer to store data received from user uart
	uint8_t *rxITBuffer[256];
	
	//This is the input buffer index. When a command is recognized, the buffer is cleaned and index is reset
	uint16_t rxITIndex = 0
	
	
	int main(void)
	{
	/**
	 *   Init your stuff
	*   You have to init two UART, one TIMER and maybe an ADC.
	*
	*/ 
	
	//Init your board stuff and dont forget these three !
	MX_USART2_UART_Init(); //This will be used to communicate with BTDevice
	MX_USART6_UART_Init(); //This will be used to communicate with user
	MX_TIM1_Init(); //This will time the timer between each data sending
	
	//Clean input buffer
	memset(rxITBuffer,0,sizeof(rxITBuffer));
	
	//Enable the receiving of the next character using interupts
	HAL_UART_Receive_IT(&huart6, &husart6RxBuffer,1);
	
	//Start TIMER used for periodical sending
	HAL_TIM_Base_Start_IT(&htim1);
	
	//Init the BTDevice drivers
	initBTDevice();
	
	//Display menu once initialization is finished
	BTDevice_displayMenu();
	
	while (1)
	{
	
	}
	}
	
	
	/**
	* Init the BTDevice drivers
	*/
	static void initBTDevice(void){
	
	BTDevice_InitTypeDef BTDevice_InitStruct;
	
	BTDevice_InitStruct.userHuart = &huart6; //userHuart
	BTDevice_InitStruct.deviceHuart = &huart2; //deviceHuart
	BTDevice_InitStruct.userInputBuffer = rxITBuffer; //input buffer
	BTDevice_InitStruct.resetInputBufferHandler = &resetInputBuffer; //reset input buffer function
	BTDevice_init(&BTDevice_InitStruct); //
	}
	
	
	/**
	* TIMER Callback function. Periodically, data will be sent.
	* Period value can be set through uart interface (userHuart)
	*/
	void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	//Make sur it's the right timer 
	if(htim->Instance == TIM1){ 
		if(BTDevice_timerCallback()){ //Is it time to send the next data ?
			//If yes, which mean we have waited the required amout of seconds
			//Then send the data !
			BTDevice_sendData(getSensorData(),SENSOR_DATA_MAX_LENGTH);
		}
	}
	}
	
	
	/**
	* Example of UART reception complete callback
	*/
	void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	//Make sur it's the right uart
	if(huart->Instance == USART6){
		storeNewValueIntoInputBuffer(); //Store newly received character into the rxITBuffer
		BTDevice_readInputBuffer(); //Parse the rxITBuffer to recognize commands
		HAL_UART_Receive_IT(&huart6,&husart6RxBuffer,1); //Enable receiving ot the new character
	}
	}
	
	
	/**
	* Example for handling incoming characters from uart input
	*/
	void storeNewValueIntoInputBuffer(void) {
	rxITBuffer[rxITIndex] = husart6RxBuffer;
	rxITIndex++;
	}
	
	
	/**
	* Example to reset the inputBuffer.
	*/
	void resetInputBuffer(void) {
	memset(rxITBuffer, 0, sizeof(rxITBuffer));
	rxITIndex = 0;
	}
	
	
	uint8_t *getSensorData(void){
	//This buffer will contain our data.
	//The free is done once the data is sent to the BTDevice
	uint8_t *dataBuffer = malloc(sizeof(uint8_t)*SENSOR_DATA_MAX_LENGTH);
	memset(dataBuffer,0,SENSOR_DATA_MAX_LENGTH);
	
	//Get your sensor data !
	uint32_t sensorValue = getSensorValue();
	
	//Now write that uint32_t into our data buffer.
	sprintf((char *)dataBuffer,"%lu",sensorValue);
	
	return dataBuffer;
	}
	



# In your stm32fxxx_it.c : 


Write the uart IRQHandle for your user uart : 

      void USART6_IRQHandler(void)
      {
      	HAL_UART_IRQHandler(&huart6);
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
