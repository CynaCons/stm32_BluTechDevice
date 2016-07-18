# stm32_BluTechDevice

#What does this driver do ?

This driver will use two UART :

	=> One will send commands to the BluTech device and receive the command's answer or receive data sent from the REST 	API (?)
	=> The other will print a menu of a list of commands that can be used to control the BluTech device. It also 			display the command's result and a lot of other useful things (such as sensor data).
	

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

I highly recommend to use the STM32CubeMX project generator. This driver uses the HAL library.
STM32CubeMX will generate the project architecture and initialize all the required peripherals.

To understand how to use, read the example files (main.c, stm32fxxx_it.c) and this readme.

Details for each function can be found in the example files (main.c, stm32fxxx_it.c)

# In main.c

- include necessary header files :
	```
		//Header file of the driver
		#include "stm32fxxx_hal_BTDevice.h" 
		
		//Include the uart related functions. Replace first x !
		#include "stm32fxxx_hal_uart.h"
		
		#include <string.h> //For string manipulation
		#include <stdlib.h> //For malloc (freeing is done in the drivers)
	```
- define the sensor data maximum length. The amout of data retrieved from the sensor should not exceed this value. You can hardcode it if you don't want to #define it
	
	```	#define SENSOR_DATA_MAX_LENGTH 128```
	
- to receive characters input from the user lets use a buffer and it's index. I also like to use a one byte buffer (husart6RxBuffer) to store user input before filling the main input buffer (rxITBuffer)
	```
		//When a command is recognized, the input buffer is filles with \0 and the index is set to 0.
		
		//Use this buffer to store data received from user uart
		uint8_t rxITBuffer[256];
		
		//Input buffer index. 
		uint16_t rxITIndex = 0;
		
		//Single byte buffer to store characters temporarily
		uint8_t husart6RxBuffer = 0;
	```

- in main.c, make sure all peripherals are intialized. To use this driver, the following peripherals must be initialized :
	- one uart to interact with the user
	- one uart to send and receive commands to the BTDevice
	- one timer to be able to send sensor data periodically

	```
		MX_USART2_UART_Init(); //This will be used to communicate with BTDevice
		MX_USART6_UART_Init(); //This will be used to communicate with user
		MX_TIM1_Init(); //This will time the timer between each data sending
	```
		
- make sure the input buffer is clean :
	
	```
		memset(rxITBuffer,0,sizeof(rxITBuffer); //fill the input buffer with \0
	```
		
- enable character receive in non blocking mode (using interupts)
	
	```
		//One character will be received in IT mode then the RxCompleteCallback will be called.
		HAL_UART_Receive_IT(&huart6, &husart6RxBuffer, 1);
	```
		
- start the timer. Timer should be configured to call the PeriodEllapsedCallback every second. It is then possible to set a custom period (like 60 seconds or other) between two data transfer using the uart commands
	
	```
		//Start timer for periodical data transfer
		HAL_TIM_Base_Start_IT(&htim1);
	```
		
- initialize the drivers using the appropriate function. To do this, simply fill the BTDevice_InitTypeDef structure and call BTDevice_init(BTDev_struct);
	
	```
		//Before main loop, initialize the drivers
		initBTDevice();

	//initBTDevice function definition
		static void initBTDevice(){
			BTDevice_InitTypeDef BTDevice_InitStruct; //Declare the structure to fill
		
			BTDevice_InitStruct.userHuart = &huart6; //userHuart
			BTDevice_InitStruct.deviceHuart = &huart2; //deviceHuart
			BTDevice_InitStruct.userInputBuffer = rxITBuffer; //input buffer
			BTDevice_InitStruct.resetInputBufferHandler = &resetInputBuffer; //reset input buffer function
			
			BTDevice_init(&BTDevice_InitStruct); 
		}
	```
		
- now that it's all initialized, display the uart commands menu ! 

	```
		BTDevice_displayMenu();
	```
	
- outside main function, write the PeriodEllapsedCallback for the timer
	
	```
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
	```
	
- write the uart RxCompleteCallback (it's important that this function is called after each character we receive, that's why we always set data length to 1 in the HAL_UART_Receive_IT function )

	
	```
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
	```
	
- the following function are example to complete the two callbacks above. 
	
	```
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
		
		 /**
		  * Example to get the data and send it as argument for the BTDevice_sendData(..) function
		  */
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
