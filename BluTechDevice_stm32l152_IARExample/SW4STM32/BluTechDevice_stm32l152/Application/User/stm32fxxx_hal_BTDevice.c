/*
 * @brief Driver to use and control a BluTech LoRa device
 * @file stm32fxxx_hal_BTDevice.c
 * @author constantin.chabirand@gmail.com
 *
 * Please refer to the tutorial and example available on github :
 * https://github.com/CynaCons/stm32_BluTechDevice
 */



/************
 * Includes
 ************/
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


/**
 * Includes specific to board model.
 * Currently implemented models are :
 *  	stm32f4-DISCO
 *  	stm32f103
 *  	stm32l152
 */
#if defined(STM32F103RCTx) || defined(STM32F103xx) || defined(STM32F103)|| defined(STM32F103xE)
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_uart.h"
#endif

#if defined(STM32F4DISCOVERY)|| defined(STM32F407VGTx)|| defined(STM32F407xx)
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#endif

#if defined(STM32L152xC) || defined(STM32L1)
#include "stm32l1xx_hal.h"
#include "stm32l1xx_hal_uart.h"
#endif

#include "stm32fxxx_hal_BTDevice.h"

/************
 * Defines
 ************/


#define NB_MENU_LINES 21 /*!< Number of lines to display in the menu */

#define DEVICE_DATA_MAX_LENGTH 256 /*!< The maximum size of data that can be sent using a BTDevice */


/********************************
 * Private functions prototypes
 ********************************/
static uint8_t checkForInputBufferReset(void);
static void displayMenu();
static void setUserUartInTimerSettingsMode(void);
static void setUserUartInDataInputMode(void);
static void setUserUartInCommandMode(void);
static void setDeviceHuart(UART_HandleTypeDef *huartx);
static void setDeviceCommandReceivedHandler(void (*fPtr)());
static void setUserHuart(UART_HandleTypeDef *huartx);
static void setUserInputBuffer(uint8_t *ptrBuffer);
static void setResetInputBufferHandler(void (*fPtr)()); //TODO All the related functions.variables have not been commented yet
static void sendDataToDevice(uint8_t *dataBuffer, uint16_t dataLength);
static void startAutoMode();
static void stopAutoMode();
static uint8_t setTimerPeriod();
static void resetPeriodCounter(void);
static void incrementPeriodCounter(void);
static uint32_t getPeriodCounter(void);
static uint32_t getTimerPeriod(void);
static void resetDeviceInputBuffer(void);


//Handlers for each command
static void autoModeOnHandler(void);
static void autoModeOffHandler(void);
static uint8_t getDataFromUser(void);
static void getTimerPeriodHandler(void);
static void getSensorValueHandler(void);
static void menuHandler(void);
static void networkJoinHandler(void);
static void rfSignalCheckHandler(void);
static void rsHandler(void);
static void sendDataHandler(void);
static void sendSampleDataHandler(void);
static void setTimerPeriodHandler(void);


/**************************************
 * Private structure and enumerations
 **************************************/

/**
 * Unique number for each command that the user can send
 */
typedef enum {
	MENU, /*!< Display the UI menu */
	AUTO_ON, /*!< Start Auto Mode */
	AUTO_OFF, /*!< Stop Auto Mode */
	SEND_DATA, /*!< Send data to the device (also on the user uart */
	GET_SENSOR_VALUE, /*!< Display the current sensor value */
	RFSIGNALCHECK, /*!< Send command to the device to perform a signal check */
	NETWORKJOIN, /*!< Send command to the device to join the BT LoRa network */
	SENDSAMPLEDATA, /*!< Send data sample to the device */
	SET_TIMER_PERIOD, /*!< Set the timer period */
	GET_TIMER_PERIOD, /*!< Returns the current timer period*/
	COMMAND_NB /*!< Total number of commands that the user can send */
} commandID;



/**
 * The different states that handle messages and command confirmations froom the BTDevice
 */
typedef enum{
	RCV_HEAD, /*!< Waiting for the first byte of the message */
	RCV_RFSIGNALCHECK, /*!< Receiving the rfsignalcheck answer */
	RCV_NETWORKJOIN, /*!< Receiving the network join answer */
	RCV_TRANSFER_ACK, /*!< Receiving the confirmation for the data transfer */
	RCV_DATA_LENGTH, /*!< Receive the length of the incoming data */
	RCV_DATA /*!< Receiving an amout of data */
} deviceUartReceiveMode;


/**
 * The specific string that the user has to write in order to perform the equivalent command.
 * See above for comments
 */
uint8_t commandArray[COMMAND_NB][32] = {
		"menu",
		"set automode on",
		"set automode off",
		"send data",
		"get sensor value",
		"rf signal check",
		"network join",
		"send sample data",
		"set timer period",
		"get timer period"
};

/**
 * State of the periodic data transfer mode
 */
typedef enum {
	AUTOMODE_OFF, /*!< OFF = no data will be sent periodically */
	AUTOMODE_ON /*!<ON = data will be sent every period. Period is set using menu */
}AutoModeStatus;


/**
 * States for the userUart
 */
typedef enum{
	COMMAND_MODE, /*!< Input buffer will be analyzed to recognized known commands in the UART RxCpltCallback*/
	TIMER_SETTINGS, /*!< User is invited to input values for the timer period */
	USER_INPUT_FINISHED, /*!< User has terminated to type it's timer period value */
	DATA_INPUT /*!< User is invited to type custom data to be sent through the BTDevice */
}UserUartMode;


/**
 * Indicator to know whether or not it is required to reset the input buffer after one character is received
 */
typedef enum{
	NO_RESET,
	RESET_BUFFER
}InputBufferStatus;


/**
 * Function pointers to each handler function which will do the action requested by the user
 */
void (*commandFunctionArray[COMMAND_NB])(void) =  {
		&menuHandler,
		&autoModeOnHandler,
		&autoModeOffHandler,
		&sendDataHandler,
		&getSensorValueHandler,
		&rfSignalCheckHandler,
		&networkJoinHandler,
		&sendSampleDataHandler,
		&setTimerPeriodHandler,
		&getTimerPeriodHandler
};


/*********************
 * Private variables
 *********************/
static uint8_t autoModeStatus = AUTOMODE_OFF; /*!< Current state of AutoMode. See enum definition*/

static UART_HandleTypeDef *deviceHuart; /*!< This uart is used to send/receive commands from the BT device using BT's UART commands */

static uint32_t periodCounter = 0; /*!< Number of seconds that have passed since last data transfer */

void (*resetInputBufferHandler)();/*!< Function pointer to the function used to reset the user input buffer */

void (*deviceCommandReceivedHandler)(uint8_t *dataBuffer, uint16_t dataLength);

static uint8_t rxDeviceBuffer[DEVICE_DATA_MAX_LENGTH]; /*!< Device Input buffer */

static uint8_t rxDeviceState = RCV_HEAD; /*!< Current state for the deviceUart. See enum definition */

static volatile uint16_t rcvDataLength = 0; /*!< Global container to store how much data will be received */

static uint32_t timerPeriod = 5; /*!<Default value for period between two transfer. Use menu to change. */

static uint8_t *userInputBuffer = NULL; /*!< Adress of the buffer that is read to analyse user's input */

static UART_HandleTypeDef *userHuart; /*!< This uart is used to communicate with the user through serial cable */

static uint8_t userUartMode = COMMAND_MODE;/*!< Current state for the userUart. See enum definition*/

static uint8_t * savedDataBuffer = NULL; /*!< Dynamicaly allocated(strdup) buffer containing the last data sent */

/*********************
 * Private functions
 *********************/


/**
 * Check the last two characters in the input buffer to see if we received an rs command
 */
static uint8_t checkForInputBufferReset(void){
	if(userInputBuffer[0] != '\0' && userInputBuffer[1] != '\0'){
		uint16_t len = strlen((const char *)userInputBuffer);
		if(userInputBuffer[len-1] == 's' && userInputBuffer[len-2] == 'r'){
			return 1;
		}
	}
	return 0;
}




/**
 * Display the application menu
 */
static void displayMenu(void){
	uint8_t *menuContent[NB_MENU_LINES] = {
			(uint8_t *)"			                APPLICATION MENU		                       	\r\n",
			(uint8_t *)"	                                                                        \r\n",
			(uint8_t *)"\r\n",
			(uint8_t *)"+++   GENERAL CONTROL\r\n",
			(uint8_t *)"    --> menu             : display this menu\r\n",
			(uint8_t *)"    --> rs               : reset the input buffer (in case of typing mistake)\r\n",
			(uint8_t *)"    --> get sensor value : display the last sensor value\r\n",
			(uint8_t *)"    --> set automode on  : start sending data periodically\r\n",
			(uint8_t *)"    --> set automode off : stop sending data periodically\r\n",
			(uint8_t *)"    --> send data        : input some ascii data that will be sent to the gateway\r\n",
			(uint8_t *)"\r\n",
			(uint8_t *)"+++   BLUTECH DEVICE CONTROL\r\n",
			(uint8_t *)"    --> rf signal check  : perform a signal check\r\n",
			(uint8_t *)"    --> network join     : join the gateway network\r\n",
			(uint8_t *)"    --> send sample data : send a sample of data for testing\r\n",
			(uint8_t *)"\r\n",
			(uint8_t *)"+++   TIMER CONTROL\r\n",
			(uint8_t *)"    --> set timer period : set the timer period\r\n",
			(uint8_t *)"    --> get timer period : get the current timer period\r\n",
			(uint8_t *)"\r\n",
			(uint8_t *)"WARNING : You have to join a network in order to send data through the LoRa module\r\n"
	};
	//Display the menu
	uint8_t menuIterator = 0;
	for(menuIterator = 0; menuIterator < NB_MENU_LINES; menuIterator++){
		HAL_UART_Transmit(userHuart,(uint8_t *)menuContent[menuIterator], strlen((const char *)menuContent[menuIterator]),10);
	}
}


/**
 * Set user uart in timer settings mode
 */
static void setUserUartInTimerSettingsMode(){
	userUartMode = TIMER_SETTINGS;
	uint8_t buffer[128];
	memset(buffer,0,sizeof(buffer));
	sprintf((char *)buffer,"\r\nUser Uart now in timer settings mode.\r\nCommands won't be recognized\r\n");
	HAL_UART_Transmit(userHuart,buffer, sizeof(buffer),10);
	memset(buffer,0,sizeof(buffer));
	sprintf((char *)buffer,"\r\nPease type the new timer period value followed by end\r\nLike this : 25 end\r\n");
	HAL_UART_Transmit(userHuart,buffer,sizeof(buffer),10);
	memset(buffer, 0, sizeof(buffer));
	sprintf((char *)buffer,"\r\nThe previous command '25 end' means 25 seconds between two data transfers\r\n");
	HAL_UART_Transmit(userHuart,buffer,sizeof(buffer),10);
}

/**
 * Set user uart in data input mode
 */
static void setUserUartInDataInputMode(){
	userUartMode = DATA_INPUT;
	uint8_t buffer[128] = "\r\nUser Uart now in data input mode.\r\nCommands won't be recognized\r\n";
	HAL_UART_Transmit(userHuart,buffer, sizeof(buffer),10);
	memset(buffer,0,sizeof(buffer));
	sprintf((char *)buffer,"Pease type the data to send followed by  \" end\"(max 64 bytes)\r\nLike this : myData end\r\n");
	HAL_UART_Transmit(userHuart,buffer,sizeof(buffer),10);
}


/**
 * Set user uart in command mode
 */
static void setUserUartInCommandMode(){
	userUartMode = COMMAND_MODE;
	uint8_t buffer[] = "\r\nUser Uart now in command mode. Commands will be recognized\r\n";
	HAL_UART_Transmit(userHuart,buffer, sizeof(buffer),10);
	displayMenu();
}


static void setResetInputBufferHandler(void (*fPtr)()){
	resetInputBufferHandler = fPtr;
}


/**
 * Set the uart to be used as user uart for the BTDevice library. This user uart is then used to display the UI and configure the board.
 * @param [in] huartx UART_HandleTypeDef's address of the uart handler to use.
 * @pre The uartx must have been initialized before in IT mode.
 */
void setUserHuart(UART_HandleTypeDef *huartx){
	userHuart = huartx;
}


/**
 * Set the uart to be used as device uart for the BTDevice library. This device uart is then used to send commands to the BluTech device.
 * @param [in] huartx UART_HandleTypeDef's address of the uart handler to use.
 * @pre The huartx must have bee initalized before.
 */
void setDeviceHuart(UART_HandleTypeDef *huartx){
	deviceHuart = huartx;
}



static void setDeviceCommandReceivedHandler(void (*fPtr)()){
	deviceCommandReceivedHandler = fPtr;

}

/**
 * Set the user input buffer's address to be check to recognize user's commands and set it's length.
 * @param [in] ptrBuffer pointer to the user input buffer.
 */
void setUserInputBuffer(uint8_t *ptrBuffer){
	userInputBuffer = ptrBuffer;
}


/**
 * The user is prompted to input some data to send through the BT device.
 * User message must be finished by " end" to signal the end-of-message.
 */
static uint8_t getDataFromUser(void){
	uint8_t tmp[64]; //buffer for the second part of the input. Must contain "end" to recognize end the end of input.
	memset(tmp,0,sizeof(tmp));
	uint8_t data[64]; //buffer for the data to be sent. Must be followed by " end"
	memset(data,0,sizeof(data));
	//Make sure enough characters have been typed
	uint16_t bufferLength  = strlen((const char *)userInputBuffer);
	if(bufferLength >= 3){
		if(userInputBuffer[bufferLength-1] == 'd' && userInputBuffer[bufferLength-2] == 'n' && userInputBuffer[bufferLength-3] == 'e'){
			sscanf((const char *)userInputBuffer,"%s %s", data, tmp);
			sprintf((char *)tmp,"The following data will be sent : ");
			HAL_UART_Transmit(userHuart, tmp, sizeof(tmp),10);
			HAL_UART_Transmit(userHuart,data,sizeof(data),10);
			memset(tmp,0,sizeof(tmp));
			sprintf((char *)tmp, "\r\n");
			HAL_UART_Transmit(userHuart, tmp, sizeof(tmp),10);
			sendDataToDevice(data, 64);
			setUserUartInCommandMode();
			return RESET_BUFFER;
		}
	}
	return NO_RESET;
}


/**
 * Return the userUartMode (which is a value of the UserUartMode enum, see header file for values)
 */
static uint8_t getUserUartMode(void){
	return userUartMode;
}

/**
 * Send a data buffer using the BT device. The sending is done using BT's UART commands and respects it's format (03 0xlength 0xData)
 * @param in dataBuffer pointer to the data buffer
 * @param in maxDataLength maximum size of the buffer
 */
static void sendDataToDevice(uint8_t * dataBuffer, uint16_t dataMaxLength){
	const uint16_t dataLength = strlen((const char *)dataBuffer)+2*sizeof(uint8_t);
	uint8_t commandBuffer[dataLength];
	uint16_t i = 0;
	for(i=0; i<dataLength; i++){
		if(i==0){
			commandBuffer[i] = 0x03;
		}else if(i == 1){
			commandBuffer[i] = (uint8_t)(strlen((const char *)dataBuffer));
		}else{
			commandBuffer[i] = dataBuffer[i-2];
		}
	}
	HAL_UART_Transmit(deviceHuart,commandBuffer,sizeof(commandBuffer),10);
	uint8_t txBuffer[dataMaxLength+sizeof("The following data was just sent : \r\n")];
	memset(txBuffer,0,sizeof(txBuffer));
	sprintf((char *)txBuffer, "The following data was just sent : ");
	strcat((char *)txBuffer,(const char *)dataBuffer);
	strcat((char *)txBuffer,"\r\n");
	HAL_UART_Transmit(userHuart,txBuffer,strlen((const char *)txBuffer),10);
	//Make a copy of the last sent data
	//If the pointer has been allocated before : Free, clean and allocate
	if(savedDataBuffer != NULL){
		free(savedDataBuffer);
		savedDataBuffer = NULL;
		savedDataBuffer = (uint8_t *)(strdup((const char *)dataBuffer));
	}else{
		//Otherwise we just allocate
		savedDataBuffer = (uint8_t *)(strdup((const char *)dataBuffer));
	}
}


/**
 * User is asked to input a new value for the periodic timer period.
 */
static uint8_t setTimerPeriod(void){
	uint8_t tmp[64];
	memset(tmp,0,sizeof(tmp));
	uint32_t tmpTimerPeriod = 0;
	uint32_t bufferLength = strlen((const char *)userInputBuffer);
	if(bufferLength >= 3){
		if(userInputBuffer[bufferLength-1] == 'd' && userInputBuffer[bufferLength -2] == 'n' && userInputBuffer[bufferLength - 3] == 'e'){
			sscanf((const char *)userInputBuffer,"%lu %s", &tmpTimerPeriod, tmp);
			timerPeriod = tmpTimerPeriod;
			memset(tmp,0,sizeof(tmp));
			sprintf((char *)tmp,"\r\nNew timer period value : %lu\r\n",timerPeriod);
			HAL_UART_Transmit(userHuart, tmp, sizeof(tmp),10);
			setUserUartInCommandMode();
			return RESET_BUFFER;
		}
	}
	return NO_RESET;
}

/**
 * Start to send temperature periodically
 */
static void startAutoMode(void){
	uint8_t txBuffer[128];
	memset(txBuffer,0,sizeof(txBuffer));
	sprintf((char *)txBuffer,"AutoMode Started !\r\n");
	HAL_UART_Transmit(userHuart,txBuffer, sizeof(txBuffer),10);
	memset(txBuffer,0,sizeof(txBuffer));
	sprintf((char *)txBuffer,"Data will now be sent periodically\r\n");
	HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
	uint32_t timerPeriodValue = getTimerPeriod();
	memset(txBuffer,0,sizeof(txBuffer));
	sprintf((char *)txBuffer,"\r\nCurrent timer period value : %lu seconds\r\n",timerPeriodValue);
	HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
	autoModeStatus = AUTOMODE_ON;
}

/**
 *   Stop to send the temperature periodically
 */
static void stopAutoMode(void){
	uint8_t txBuffer[128];
	memset(txBuffer,0,sizeof(txBuffer));
	sprintf((char *)txBuffer,"\r\nAutoMode Stopped !\r\nData will not be sent periodically\r\n");
	HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
	autoModeStatus = AUTOMODE_OFF;
}


/******************************
 * Commands Handler functions
 ******************************/

/**
 * Code and functions titles are self-explanatory
 */
static void menuHandler(void){
	displayMenu();
}

static void rsHandler(void){
	uint8_t txBuffer[128];
	memset(txBuffer,0,sizeof(txBuffer));
	sprintf((char *)txBuffer,"\r\nInput buffer reset. Start typing a new command\r\n");
	HAL_UART_Transmit(userHuart,txBuffer, sizeof(txBuffer),10);
}

static void autoModeOnHandler(void){
	startAutoMode();
}

static void autoModeOffHandler(void){
	stopAutoMode();
}


static void sendDataHandler(void){
	setUserUartInDataInputMode();

}

static void getSensorValueHandler(void){
	uint8_t txBuffer[128];
	memset(txBuffer,0,sizeof(txBuffer));
	if(savedDataBuffer != NULL){
		sprintf((char *)txBuffer,"\r\nThis is the last sent data :\r\n%s\r\n",savedDataBuffer);
	}else{
		sprintf((char *)txBuffer,"\r\nNo data has been sent yet\r\n");
	}
	HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
}


static void rfSignalCheckHandler(void){
	uint8_t command[3];
	uint8_t buffer[] = "\r\nrfsignalcheck command was sent\r\n";
	command[0] = 0x01; command[1] = 0x01; command[2] = 0x01;
	HAL_UART_Transmit(deviceHuart,command,sizeof(command),10);
	HAL_UART_Transmit(userHuart, buffer, sizeof(buffer),10);
}


static void networkJoinHandler(void){
	uint8_t command[3];
	uint8_t buffer[] = "\r\nnetworkjoin command was sent\r\n";
	command[0] = 0x02; command[1] = 0x01; command[2] = 0x01;
	HAL_UART_Transmit(deviceHuart, command,sizeof(command),10);
	HAL_UART_Transmit(userHuart, buffer,sizeof(buffer),10);
}


static void sendSampleDataHandler(void){
	uint8_t command[7];
	command[0] = 0x01; command[1] = 0x05; command[2] = 0x01;
	command[3] = 0x02; command[4] = 0x03; command[5] = 0x04;
	command[6] = 0x05;
	HAL_UART_Transmit(deviceHuart, command,sizeof(command),10);
	uint8_t txBuffer[64];
	memset(txBuffer,0,sizeof(txBuffer));
	sprintf((char *)txBuffer,"\r\nThis data was sent : %d %d %d %d %d\r\n",command[2],command[3],
			command[4],command[5],command[6]);
	HAL_UART_Transmit(userHuart,txBuffer, sizeof(txBuffer), 10);;
}

static void getTimerPeriodHandler(void){
	uint32_t timerPeriodValue = getTimerPeriod();
	uint8_t txBuffer[64];
	memset(txBuffer,0,sizeof(txBuffer));
	sprintf((char *)txBuffer,"\r\nCurrent timer period value : %lu seconds\r\n",timerPeriodValue);
	HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
}

static void setTimerPeriodHandler(void){
	setUserUartInTimerSettingsMode();
}


/**
 * Returns the current timer period
 */
static uint32_t getTimerPeriod(){
	return timerPeriod;
}


/**
 * Returns the current number of seconds waited since last data transfer
 */
static uint32_t getPeriodCounter(){
	return periodCounter;
}


/**
 * Returns current auto mode status (ON or OFF)
 */
static uint8_t getAutoModeStatus(void){
	return autoModeStatus;
}


/**
 * Add one to the number of seconds waited since last data transfer
 */
static void incrementPeriodCounter(){
	periodCounter++;
}



/**
 * Reset the number of seconds waited since last data transfer
 */
static void resetPeriodCounter(){
	periodCounter = 0;
}

/**
 * This function reset the buffer filled by messages received from the device via deviceUart
 */
static void resetDeviceInputBuffer(void){
	memset(rxDeviceBuffer,0,sizeof(rxDeviceBuffer));
}


/*****************************
 * BTDevice public functions
 *****************************/


/**
 * This function handle the communication from the BTDevice to the MCU
 * First, we receive the "head" (the first byte). Depending on the head value,
 * we start IT reception process for the rest of the message.
 * Once the message is completely received, we then display it to the user
 * through the userUart.
 */
//TODO : Sometimes when trying to receive the body of a command we probably receive some other commands at the same time.
//TODO : Some kind of message queue would be nice to make sure we handle responses in the right order
//TODO : The right solution  would probably be to store all the incoming bytes on IT and then read what we received in a queued manner
void BTDevice_deviceUartCallback(uint8_t *deviceUartRxBuffer){
	uint8_t txBuffer[64];
	memset(txBuffer,0,sizeof(txBuffer));
	switch(rxDeviceState){
	case RCV_HEAD : //Receiving the head of the message
		switch( (*deviceUartRxBuffer) ){
		case 0xfc : //When BluTech board is starting. Ignore this byte and wait for another.
		case 0x00 :
			rxDeviceState = RCV_HEAD;
			HAL_UART_Receive_IT(deviceHuart,deviceUartRxBuffer,1);
			break;
		case 0x01 : //rfsignalcheck
			rxDeviceState = RCV_RFSIGNALCHECK;
			HAL_UART_Receive_IT(deviceHuart,rxDeviceBuffer,4);
			break;
		case 0x02 :
			rxDeviceState = RCV_NETWORKJOIN;
			HAL_UART_Receive_IT(deviceHuart,rxDeviceBuffer,2);
			break;
		case 0x03 :
			rxDeviceState = RCV_TRANSFER_ACK;
			HAL_UART_Receive_IT(deviceHuart,rxDeviceBuffer,2);
			break;
		case 0x04 :
			rxDeviceState = RCV_DATA_LENGTH;
			HAL_UART_Receive_IT(deviceHuart,deviceUartRxBuffer,1);
			break;
		default :
			rxDeviceState = RCV_HEAD;
			HAL_UART_Receive_IT(deviceHuart, deviceUartRxBuffer,1);
			break;
		}
		break;
		case RCV_RFSIGNALCHECK : //Receiving a confirmation following a rfsignalcheck command

			sprintf((char *)txBuffer,"RF signal check answer was received\r\n");
			HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
			memset(txBuffer,0,sizeof(txBuffer));
			sprintf((char *)txBuffer,"RSSI : %x%x SNR : %x\r\n",rxDeviceBuffer[1],rxDeviceBuffer[2],rxDeviceBuffer[3]);
			HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
			rxDeviceState = RCV_HEAD;
			HAL_UART_Receive_IT(deviceHuart,deviceUartRxBuffer,1);
			break;
		case RCV_NETWORKJOIN : //Receiving a confirmation following a network join command

			if(rxDeviceBuffer[1] == 0x02){
				sprintf((char *)txBuffer,"Network Join success\r\n");
				HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
			}else{
				sprintf((char *)txBuffer,"Network Join failed\r\n");
				HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
			}
			rxDeviceState = RCV_HEAD;
			HAL_UART_Receive_IT(deviceHuart,deviceUartRxBuffer,1);
			break;
		case RCV_TRANSFER_ACK: //Receiving a confirmation following a data transfer
			if(rxDeviceBuffer[1] == 0x01){
				sprintf((char *)txBuffer,"Data Transfer success\r\n");
				HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
			}else{
				sprintf((char *)txBuffer,"Data Transfer failed\r\n");
				HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
			}
			rxDeviceState = RCV_HEAD;
			HAL_UART_Receive_IT(deviceHuart,deviceUartRxBuffer,1);
			break;
		case RCV_DATA_LENGTH : //Receiving the body length of the data message
			rcvDataLength = *deviceUartRxBuffer;
			HAL_UART_Receive_IT(deviceHuart,rxDeviceBuffer,rcvDataLength);
			rxDeviceState = RCV_DATA;
			break;
		case RCV_DATA : // Receiving the body of a data message
			HAL_UART_Receive_IT(deviceHuart,deviceUartRxBuffer,1);
			deviceCommandReceivedHandler(rxDeviceBuffer, rcvDataLength);
			rcvDataLength = 0;
			rxDeviceState = RCV_HEAD;
			break;
	}
	resetDeviceInputBuffer();
}


/**
 * Public wrapper around the static function displayMenu to display the use menu via userUart
 */
void BTDevice_displayMenu(void){
	displayMenu();
}


/**
 * This function initializes all the required references. Call this before doing anything else.
 * The structure must be filled with references to the peripherals that should be used by this driver
 */
void BTDevice_init(BTDevice_InitTypeDef *BTDevice_InitStruct){
	setUserHuart(BTDevice_InitStruct->userHuart);
	setDeviceHuart(BTDevice_InitStruct->deviceHuart);
	setUserInputBuffer(BTDevice_InitStruct->userInputBuffer);
	setResetInputBufferHandler(BTDevice_InitStruct->resetInputBufferHandler);
	setDeviceCommandReceivedHandler(BTDevice_InitStruct->deviceCommandReceivedHandler);
}


/**
 * This function is called each time the user input a character in the userUart.
 * There are several differents behavior according to the current userUart mode.
 * In COMMAND_MODE : user input buffer is analysed to recognize a known command.
 * 		If a command is recognized, the input buffer is reset and the corresponding
 * 		command handler is called.
 * In DATA_INPUT : user is prompted to input custom data to send via the BTDevice
 * 		User's data message must be finished with " end" to signal the end of the input process.
 * 		Then the adequate function is called to send the data
 * In TIMER_SETTINGS : user is prompted to input a value for the timer period. To signal the end
 * 		of the input process, user must type " end". The value is then used to time data transfers
 */
void BTDevice_readInputBuffer(){
	uint8_t commandIterator;
	uint8_t commandRecognized = 0;
	uint16_t len = strlen((const char *)userInputBuffer);
	HAL_UART_Transmit(userHuart, &userInputBuffer[len-1],sizeof(uint8_t),10);
	switch(getUserUartMode()){
	case  COMMAND_MODE:
		if(checkForInputBufferReset() == 1){
			rsHandler();
			resetInputBufferHandler();
		}else{
			for(commandIterator = 0; commandIterator < COMMAND_NB; commandIterator++){
				if(strcmp((const char *)commandArray[commandIterator],(const char *)userInputBuffer) == 0){
					commandRecognized = 1;
					break;
				}
			}
		}
		if(commandRecognized == 1){
			(commandFunctionArray[commandIterator])();
			resetInputBufferHandler();
		}
		break;
	case DATA_INPUT:
		if(getDataFromUser() == RESET_BUFFER){
			resetInputBufferHandler();
		}
		break;
	case TIMER_SETTINGS:
		if(setTimerPeriod() == RESET_BUFFER){
			resetInputBufferHandler();
		}
		break;
	}
}


/**
 * This function send a data buffer via the BTDevice.
 * A copy of the data is displayed via the userUart.
 * When automode is turned ON, this function is called periodically.
 */
void BTDevice_sendData(uint8_t *dataBuffer, uint16_t dataMaxLength){

	uint8_t txBuffer[64];
	memset(txBuffer,0, sizeof(txBuffer));
	if(getAutoModeStatus() == AUTOMODE_ON){
		sprintf((char *)txBuffer,"AutoMode is ON\r\n");
		HAL_UART_Transmit(userHuart, txBuffer,sizeof(txBuffer),10);
	}else{
		sprintf((char *)txBuffer,"AutoMode is OFF\r\n");
		HAL_UART_Transmit(userHuart, txBuffer,sizeof(txBuffer),10);
	}
	sendDataToDevice(dataBuffer,dataMaxLength);
}


/**
 * This function must be called inside the timer up callback.
 * First, the number of seconds waited since last data transfer is increased
 * Then, this number of second is compared to the timer period value (set with user menu)
 * If enough time has been waited since last data transfer, reset the counter and return 1.
 */
uint8_t BTDevice_timerCallback(){
	if(getAutoModeStatus() == AUTOMODE_ON){
		incrementPeriodCounter();
		if(getPeriodCounter() >= getTimerPeriod()){
			resetPeriodCounter();
			return 1;
		}
	}
	return 0;
}



