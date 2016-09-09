/*
 * @brief Library to use and control a BluTech LoRa device
 * @file stm32fxxx_hal_BTDevice.c
 * @author constantin.chabirand@gmail.com
 *
 * Please refer to the tutorial and example available on github :
 * https://github.com/CynaCons/stm32_BluTechDevice
 */

//HARD LINK TEST

/************
 * Includes
 ************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

#include "stm32fxxx_hal_BTDevice.h" //Header file for this library

/************
 * Defines
 ************/

#define NB_MENU_LINES 27 /*!< Number of lines to display in the menu */

#define DEVICE_DATA_MAX_LENGTH 256 /*!< The maximum size of data that can be received by a BTDevice. This is not a choice, see BT specs */

#define USER_RX_BUFFER_LENGTH 128 /*!< Length of the buffer used to store user input */

#define NB_FAILED_INIT 5 /*!< During auto-init process, networkJoin request is sent every 10 seconds untill the device join the network.
								After NB_FAILED_INIT failed requests, there will be an error (if value = 5, then after 5*10 seconds there will be an error */

#define SENSOR_DATA_MAX_LENGTH 64 /*!< Maximum length for the payload */


/************
 * Macros
 ************/


/********************************
 * Private functions prototypes
 ********************************/
static uint8_t checkForInputBufferReset(void);
static void checkSensorData(uint8_t *tmp);
static void displayMenu();
static uint8_t getAutoModeStatus(void);
static uint32_t getPeriodCounter(void);
static uint32_t getTimerPeriod(void);
static void incrementPeriodCounter(void);
static void resetPeriodCounter(void);
static void resetDeviceInputBuffer(void);
static void sendDataToDevice(uint8_t *dataBuffer, uint16_t dataBufferLength);
static void setUserUartInTimerSettingsMode(void);
static void setUserUartInDataInputMode(void);
static void setUserUartInCommandMode(void);
static void setDeviceHuart(UART_HandleTypeDef *huartx);
static void setDeviceCommandReceivedHandler(void (*fPtr)(uint8_t *dataBuffer, uint16_t dataLength));
static void setSignalEventFunction(void (*fPtr)(void *));
static void setUserHuart(UART_HandleTypeDef *huartx);
static void setSensorDataFunction(uint8_t* (*fPtr)(void));
static uint8_t setTimerPeriod();
static void startAutoMode();
static void stopAutoMode();
static void resetInputBuffer();
static void storeNewValueIntoInputBuffer(uint8_t newCharacter);
static void ThrowError(BTDevice_Error errorCode);


//Handlers for each command
static void autoModeOnHandler(void);
static void autoModeOffHandler(void);
static void deviceStatusHandler(void);
static void flushSensorDataHandler(void);
static uint8_t getDataFromUser(void);
static void getAutoModeStatusHandler(void);
static void getNetworkStatusHandler(void);
static void getLastValueHandler(void);
static void getTimerPeriodHandler(void);
static void menuHandler(void);
static void networkJoinHandler(void);
static void rfSignalCheckHandler(void);
static void rsHandler(void);
static void sendDataHandler(void);
static void sendSampleDataHandler(void);
static void setTimerPeriodHandler(void);

//Error handling related functions
static void ThrowError(BTDevice_Error errorCode);
static void forwardErrorToServer(uint8_t *errorMessage, uint16_t errorMessageLength);

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
	GET_LAST_VALUE, /*!< Display the current sensor value */
	RFSIGNALCHECK, /*!< Send command to the device to perform a signal check */
	NETWORKJOIN, /*!< Send command to the device to join the BT LoRa network */
	SENDSAMPLEDATA, /*!< Send data sample to the device */
	SET_TIMER_PERIOD, /*!< Set the timer period */
	GET_TIMER_PERIOD, /*!< Returns the current timer period*/
	GET_NETWORK_STATUS, /*!< Returns the current state of network (joined or not)*/
	FLUSH_SENSOR_DATA, /*!< Force the sending of sensor data and reset the period counter */
	GET_AUTOMODE_STATUS, /*!< Display the current AutoMode state (ON or OFF) */
	DEVICE_STATUS, /*!< Display numerous indicators and states of the devices */
	COMMAND_NB/*!< Total number of commands that the user can send */
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
		"get last value",
		"rf signal check",
		"network join",
		"send sample data",
		"get timer period",
		"set timer period",
		"get network status",
		"flush sensor data",
		"get automode status",
		"device status"
};



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
 * Indicator to know whether the device is connected to the gateway's network or not.
 * It should be automatically updated by the library
 */
typedef enum{
	NETWORK_JOIN_ERROR = 0,
	NETWORK_JOIN_OK
}NetworkJoinStatus;

/**
 * Function pointers to each handler function which will do the action requested by the user
 */
void (*commandFunctionArray[COMMAND_NB])(void) =  {
		&menuHandler,
		&autoModeOnHandler,
		&autoModeOffHandler,
		&sendDataHandler,
		&getLastValueHandler,
		&rfSignalCheckHandler,
		&networkJoinHandler,
		&sendSampleDataHandler,
		&getTimerPeriodHandler,
		&setTimerPeriodHandler,
		&getNetworkStatusHandler,
		&flushSensorDataHandler,
		&getAutoModeStatusHandler,
		&deviceStatusHandler
};
/*********************
 * Private variables
 *********************/
static uint8_t autoModeStatus = AUTOMODE_OFF; /*!< Current state of AutoMode. See enum definition*/

static UART_HandleTypeDef *deviceHuart; /*!< This uart is used to send/receive commands from the BT device using BT's UART commands */

static uint32_t periodCounter = 0; /*!< Number of seconds that have passed since last data transfer */

void (*deviceCommandReceivedHandler)(uint8_t *dataBuffer, uint16_t dataLength); /*!< Function pointer the function
that will process the received data/command (received by the node from the gateway)*/

static uint8_t flushSensorDataFlag = 0;

static uint32_t lastCommandTick = 0; //SysTick value (HAL_GetTick) corresponding to the last command/data received processing. Anti-Spam.

static uint8_t networkJoinStatus = NETWORK_JOIN_ERROR; /*<! Current state of the network : NETWORK_JOIN_OK or NETWORK_JOIN_ERROR */

static uint8_t rxDeviceBuffer[DEVICE_DATA_MAX_LENGTH]; /*!< Device Input buffer */

static uint8_t rxDeviceState = RCV_HEAD; /*!< Current state for the deviceUart. See enum definition */

static volatile uint16_t rcvDataLength = 0; /*!< Global container to store how much data will be received */

static uint32_t timerPeriod = 5; /*!<Default value for period between two transfer. Use menu to change or AutoInit procedure */

static UART_HandleTypeDef *userHuart; /*!< This uart is used to communicate with the user through serial cable */

static uint8_t userUartMode = COMMAND_MODE;/*!< Current state for the userUart. See enum definition*/

static uint8_t * savedDataBuffer = NULL; /*!< Dynamicaly allocated(strdup) buffer containing the last data sent */ 

static uint8_t userUartCallbackFlag = 0; /*!< Flag to signal a character was received on userUart. It triggers received data analysis */

static uint8_t deviceUartCallbackFlag = 0; /*!< Flag to signal a character was received on deviceUart. It triggers received data analysis */

static uint8_t timerCallbackFlag = 0; /*!< Flag to signal a second has ellapsed. It triggers data sending when enough time has bee waited */

static uint8_t *localDeviceUartRxBuffer = NULL; /*!< Address of the syngle-byte buffer used to receive user input */

static uint8_t* (*getSensorData)(void); /*!< Function pointer to the function that will return sensor data in JSON format */

static void (*signalEventFunction)(void *); /*!< Function pointer to the function to be called once data has been sent */

static uint32_t lastInterruptTick = 0; /*!< Used in order to create a short delay between a character received on userUart and analysis of the data*/

static uint8_t rfSignalCheckCopy[3] = ""; /*!< Used to store the last  signal information. Three bytes to store : RSSI (2 bytes) and SNR (1 byte) */

static uint8_t userInputBuffer[USER_RX_BUFFER_LENGTH] = ""; /*!< Buffer to store the characters typed by the user */

static uint16_t userInputBufferIndex = 0; /*!< Current index of where to write in the userInputBuffer */

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
 * Analyse a buffer : check if it is JSON formatted, if the dataBuffer won't cause an overflow ( more than 128 bytes )
 * and if the sensor values in the JSON are good (if value = 0 it is considered bad)
 *
 * @param [in] tmp the buffer
 */
static void checkSensorData(uint8_t *tmp){
	//Warning : pointer p is incremented in the loops' condition
	//*p++  means that p value is used for the test, and then it is incremented)

	uint8_t *p = tmp; //p points to the beginning of the JSON string
	uint8_t sensorUnresponsive = 1; //Assume the values are wrong (==0) and we must send an error

	while(*p != '}' && (p-tmp) < SENSOR_DATA_MAX_LENGTH){
		//Jump to firt semi-column
		while(*p++ != ':' && (p-tmp) < SENSOR_DATA_MAX_LENGTH)
			;
		if((p-tmp) == SENSOR_DATA_MAX_LENGTH)
			break;
		p++; //Skip "
		//check if the json field. If it's not a number, dont sent an error
		//If it's a number and it's != 0, then dont send an error
		if(!isdigit(*p) || (isdigit(*p) && atoi((const char *)p) != 0)){
			sensorUnresponsive = 0; //Found a sensor value != 0, do not send the error
		}

		//Jump to next comma or end of json
		while(*p++ != ',' && *p != '}' && (p-tmp) < SENSOR_DATA_MAX_LENGTH)
			;
	}


	if(sensorUnresponsive == 1)
		ThrowError(BTERROR_SENSOR_UNRESPONSIVE);
	if((p-tmp) >= SENSOR_DATA_MAX_LENGTH)
		ThrowError(BTERROR_SENSOR_DATA_FORMAT);
	else
		if((p-tmp) >= SENSOR_DATA_MAX_LENGTH)
			ThrowError(BTERROR_DEVICE_PAYLOAD_OVFL);
}


/**
 * Display the application menu and commands name
 */
static void displayMenu(void){
	//Static strings for the menu
	const uint8_t *menuContent[NB_MENU_LINES] = {
			(uint8_t *)"= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = \r\n",
			(uint8_t *)"			                APPLICATION MENU		                       	\r\n",
			(uint8_t *)"	                                                                        \r\n",
			(uint8_t *)"\r\n",
			(uint8_t *)"+++   GENERAL CONTROL\r\n",
			(uint8_t *)"    --> menu                : display this menu\r\n",
			(uint8_t *)"    --> device status       : display a status report for this device\r\n",
			(uint8_t *)"    --> rs                  : reset the input buffer (in case of typing mistake)\r\n",
			(uint8_t *)"    --> get last value      : display the last sensor value\r\n",
			(uint8_t *)"    --> set automode on     : start sending data periodically\r\n",
			(uint8_t *)"    --> set automode off    : stop sending data periodically\r\n",
			(uint8_t *)"    --> get automode status : display the automode status (ON or OFF)\r\n",
			(uint8_t *)"\r\n",
			(uint8_t *)"+++   BLUTECH DEVICE CONTROL\r\n",
			(uint8_t *)"    --> rf signal check     : perform a signal check\r\n",
			(uint8_t *)"    --> network join        : join the gateway network\r\n",
			(uint8_t *)"    --> get network status  : display the network status (CONNECTED or not)\r\n",
			(uint8_t *)"    --> flush sensor data   : instantly send sensor data and reset the timer period\r\n",
			(uint8_t *)"    --> send data           : input some ascii data that will be sent to the gateway\r\n",
			(uint8_t *)"    --> send sample data    : send a sample of data for testing\r\n",
			(uint8_t *)"\r\n",
			(uint8_t *)"+++   TIMER CONTROL\r\n",
			(uint8_t *)"    --> set timer period    : set the timer period\r\n",
			(uint8_t *)"    --> get timer period    : get the current timer period\r\n",
			(uint8_t *)"\r\n",
			(uint8_t *)"WARNING : You have to join a network in order to send data through the LoRa module\r\n",
			(uint8_t *)"= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = \r\n",

	};

	//Display the menu
	uint8_t menuIterator = 0;
	for(menuIterator = 0; menuIterator < NB_MENU_LINES; menuIterator++){
		HAL_UART_Transmit(userHuart,(uint8_t *)menuContent[menuIterator], strlen((const char *)menuContent[menuIterator]),10);
	}
}


/**
 * Return the value of networkJoinStatus variable.
 * If connected, returns NETWORK_JOIN_OK. If not, returns NETWORK_JOIN_ERROR
 */
static uint8_t getNetworkJoinStatus(void){
	return networkJoinStatus;
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


/**
 * Set the function pointer for the deviceCommandReceivedHandler
 *		#The function that should process the received data/command should be written in main.c
 *		#This's function's pointer should then be passed to the library during Init so that the library can call
 *			it when receiving data/from from the gateway.
 *			The handling fonction can then be called from the library using "deviceCommandReceivedHandler();"
 *		#This function takes two arguments described in the following @param fields
 *	@param dataBuffer pointer to a buffer of characters containing the received data/command
 *	@param dataLength the length of the data/command contained in the buffer
 */
static void setDeviceCommandReceivedHandler(void (*fPtr)(uint8_t *dataBuffer, uint16_t dataLength))
{
	deviceCommandReceivedHandler = fPtr;

}


/**
 * Set the function pointer for the function that will returns the sensor data correctly formated (JSON format)
 * The function can be exectuted in the library by a direct call to the function pointer's name "getSensorData()"
 *
 * @param [in] fPtr pointer to the function's address
 * @pre the function pointed by the pointer must be defined in user space
 */
static void setSensorDataFunction(uint8_t* (*fPtr)(void))
{
	getSensorData = fPtr;
}



/**
 * Set the function pointer for the function that will be exectuted when the sensor data has been sent
 * The function can be executed in the library by a direct call to the function pointer's name "dataSentCallback()"
 *
 * @param [in] fPtr pointer to the function's address
 * @pre the function pointed by the pointer must be defined in user space
 */
static void setSignalEventFunction(void (*fPtr)(void *))
{
	signalEventFunction = fPtr;
}

//TODO Should test reset functionnality and bug-free ASAP
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
	if(bufferLength >= 2 && userInputBuffer[bufferLength-1] == 's' && userInputBuffer[bufferLength-2] == 'r'){
		sprintf((char *)tmp,"RESET INPUT\r\n");
		HAL_UART_Transmit(userHuart,tmp,strlen((const char *)tmp),10);
		memset(tmp, 0, sizeof(tmp));
		memset(userInputBuffer, 0, sizeof(userInputBuffer));
		userInputBufferIndex = 0;
	}
	else if(bufferLength >= 3){
		if(userInputBuffer[bufferLength-1] == 'd' && userInputBuffer[bufferLength-2] == 'n' && userInputBuffer[bufferLength-3] == 'e'){
			strncpy((char *)data, (const char *)userInputBuffer, bufferLength -4);
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
static void sendDataToDevice(uint8_t * dataBuffer, uint16_t dataBufferLength){
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
	uint8_t txBuffer[dataBufferLength+sizeof("The following data was just sent : \r\n")];
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
	sprintf((char *)txBuffer,"Current timer period value : %lu seconds\r\n",timerPeriodValue);
	HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
	autoModeStatus = AUTOMODE_ON;
}

/**
 *   Stop to send the temperature periodically
 */
static void stopAutoMode(void){
	uint8_t txBuffer[128];
	memset(txBuffer,0,sizeof(txBuffer));
	sprintf((char *)txBuffer,"AutoMode Stopped !\r\nData will not be sent periodically\r\n");
	HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
	autoModeStatus = AUTOMODE_OFF;
}


/******************************
 * Commands Handler functions
 ******************************/

/**
 * Code and functions titles are self-explanatory. No need for comments.
 */
static void menuHandler(void){
	displayMenu();
}

static void rsHandler(void){
	uint8_t txBuffer[128];
	memset(txBuffer,0,sizeof(txBuffer));
	sprintf((char *)txBuffer,"Input buffer reset. Start typing a new command\r\n");
	HAL_UART_Transmit(userHuart,txBuffer, sizeof(txBuffer),10);
}

static void autoModeOnHandler(void){
	startAutoMode();
}

static void autoModeOffHandler(void){
	stopAutoMode();
}

static void flushSensorDataHandler(void){
	uint8_t txBuffer[128];
	memset(txBuffer,0, sizeof(txBuffer));
	flushSensorDataFlag = 1;
	sprintf((char *)txBuffer,"Flushing the sensor data...\r\n");
	HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
}

static void getAutoModeStatusHandler(void){
	uint8_t txBuffer[128];
	memset(txBuffer,0,sizeof(txBuffer));
	if(getAutoModeStatus() == AUTOMODE_OFF){
		sprintf((char *)txBuffer, "AutoMode is OFF\r\n");
		HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
	}else{
		sprintf((char *)txBuffer, "AutoMode is ON\r\n");
		HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
	}
}

//Add number of seconds before next sending
static void deviceStatusHandler(void)
{
	uint8_t txBuffer[128];
	memset(txBuffer,0,sizeof(txBuffer));
	sprintf((char *)txBuffer,"\r\n . . . . . . . .Device status report BEGINNING:. . . . . . . .\r\n");
	HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
	getAutoModeStatusHandler();
	getTimerPeriodHandler();
	getNetworkStatusHandler();
	memset(txBuffer,0,sizeof(txBuffer));
	if(rfSignalCheckCopy[0] != '\0')
		sprintf((char *)txBuffer,"Signal info : RSSI %x%x , SNR %x\r\n",rfSignalCheckCopy[0],rfSignalCheckCopy[1],rfSignalCheckCopy[2]);
	else
		sprintf((char *)txBuffer,"Signal info : no signal info available. Do \"rf signal check \" first\r\n");
	HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
	getLastValueHandler();
	memset(txBuffer,0,sizeof(txBuffer));
	sprintf((char *)txBuffer,". . . . . . . .Device status report END. . . . . . . .\r\n");
	HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
}


static void sendDataHandler(void){
	setUserUartInDataInputMode();
}


static void getLastValueHandler(void){
	uint8_t txBuffer[128];
	memset(txBuffer,0,sizeof(txBuffer));
	if(savedDataBuffer != NULL){
		sprintf((char *)txBuffer,"This is the last sent data :%s\r\n",savedDataBuffer);
	}else{
		sprintf((char *)txBuffer,"No data has been sent yet\r\n");
	}
	HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
}


static void rfSignalCheckHandler(void){
	uint8_t command[3];
	uint8_t buffer[] = "rfsignalcheck command was sent\r\n";
	command[0] = 0x01; command[1] = 0x01; command[2] = 0x01;
	HAL_UART_Transmit(deviceHuart,command,sizeof(command),10);
	HAL_UART_Transmit(userHuart, buffer, sizeof(buffer),10);
}


static void networkJoinHandler(void){
	uint8_t command[3];
	uint8_t buffer[] = "networkjoin command was sent\r\n";
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
	sprintf((char *)txBuffer,"This data was sent : %d %d %d %d %d\r\n",command[2],command[3],
			command[4],command[5],command[6]);
	HAL_UART_Transmit(userHuart,txBuffer, sizeof(txBuffer), 10);;
}

static void getNetworkStatusHandler(void){
	uint8_t txBuffer[128];
	memset(txBuffer,0,sizeof(txBuffer));
	if(getNetworkJoinStatus() == NETWORK_JOIN_OK){
		sprintf((char *)txBuffer,"Current network status : NETWORK JOIN OK\r\n");
		HAL_UART_Transmit(userHuart, txBuffer, sizeof(txBuffer),10);
	}else{
		sprintf((char *)txBuffer, "Current network status : NETWORK JOIN ERROR\r\n");
		HAL_UART_Transmit(userHuart, txBuffer, sizeof(txBuffer),10);
	}

}

static void getTimerPeriodHandler(void){
	uint32_t timerPeriodValue = getTimerPeriod();
	uint8_t txBuffer[64];
	memset(txBuffer,0,sizeof(txBuffer));
	sprintf((char *)txBuffer,"Current timer period value : %lu seconds\r\n",timerPeriodValue);
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


/**
 * Fill the user input buffer with zeroes and set it's index to zero.
 */
static void resetInputBuffer() {
	memset(userInputBuffer, 0, sizeof(userInputBuffer));
	userInputBufferIndex = 0;
}


/*
 * Store the received character from user input into the buffer
 * This routine will be called by the UART Callback
 * @pre rxITIndex must be initialized to zero
 * @pre rxITBuffer should be filled with zeroes
 */
void storeNewValueIntoInputBuffer(uint8_t newCharacter) {
	userInputBuffer[userInputBufferIndex] = newCharacter;
	userInputBufferIndex++;
}


/*****************************
 * BTDevice public functions
 *****************************/


/*
 * This function will be called by BTDevice_intitLoop() when a device must be initialized on startup without further configuration.
 * This function will set the timer period value, the automode status and will perform a network join
 *
 * @param [in] defaultValues the structure containing initialisation parameters
 * @retVal BTDevice_OK if connected, BTDevice_ERROR otherwise
 */
BTDevice_Status autoInitWithDefaultValues(BTDevice_AutoInitTypeDef defaultValues){
	static uint32_t startTick = 0;
	static uint8_t txBuffer[256];

	//To keep track of the number of times the initialization failed. Incremented every ~10 seconds
	static uint16_t numberOfFailedTests = 0;

	//At first execution, set the timer and AutoMode status
	if(startTick == 0){
		//Set the new timer value with the parameter
		timerPeriod = defaultValues.timerPeriodValue;

		//Set the AutoMode status with the parameter
		if(defaultValues.autoModeStatus == AUTOMODE_OFF)
			autoModeOffHandler();
		else
			autoModeOnHandler();
	}
	//Every 10 seconds, perform a network join. This behaviour should be repeated untill the device is connected.
	if((HAL_GetTick() - startTick) >= 20000 || startTick == 0){
		memset(txBuffer,0,sizeof(txBuffer));
		sprintf((char *)txBuffer,"\r\n****************Launching AutoInit Procedure****************\r\n");
		HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);

		//Send a network join request and save current time
		startTick = HAL_GetTick();
		networkJoinHandler();

		//Keep track of the number of failed tests, if >=5 (~50 seconds) it will throw an error
		if(numberOfFailedTests++ >= NB_FAILED_INIT) ThrowError(BTERROR_AUTOINIT_TIMEOUT); //Check the current value and increment for next check


	}

	//Check the network join status. If connected, returns BTDevice_OK which should break the loop
	if(getNetworkJoinStatus() == NETWORK_JOIN_OK){
		memset(txBuffer,0,sizeof(txBuffer));
		sprintf((char *)txBuffer,"****************AutoInit Procedure Succes****************\r\n");
		HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
		return BTDevice_OK;
	}
	return BTDevice_ERROR;
}


/**
 * This function handle the communication from the BTDevice to the MCU
 *
 * The "head" (the first byte) is recevied first. Depending on the head value, start IT reception process for the rest of the message.
 * Once the message is completely received, it is displayed to the user through the userUart and reception for the next messsage is enabled
 */
void deviceUartCallback(uint8_t *deviceUartRxBuffer){
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
			rfSignalCheckCopy[0] = rxDeviceBuffer[1];
			rfSignalCheckCopy[1] = rxDeviceBuffer[2];
			rfSignalCheckCopy[2] = rxDeviceBuffer[3];
			rxDeviceState = RCV_HEAD;

			//Verify that the data is correct (all three bytes  != 0), if not throw error
			if(rxDeviceBuffer[0] == 0 && rxDeviceBuffer[1] == 0 && rxDeviceBuffer[2] == 0)
				ThrowError(BTERROR_BAD_SIGNAL);
			HAL_UART_Receive_IT(deviceHuart,deviceUartRxBuffer,1);
			break;
		case RCV_NETWORKJOIN : //Receiving a confirmation following a network join command

			if(rxDeviceBuffer[1] == 0x02){
				sprintf((char *)txBuffer,"Network Join success\r\n");
				HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
				networkJoinStatus = NETWORK_JOIN_OK;
			}else{
				sprintf((char *)txBuffer,"Network Join failed\r\n");
				HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
				networkJoinStatus = NETWORK_JOIN_ERROR;
			}
			rxDeviceState = RCV_HEAD;
			HAL_UART_Receive_IT(deviceHuart,deviceUartRxBuffer,1);
			break;
		case RCV_TRANSFER_ACK: //Receiving a confirmation following a data transfer
			if(rxDeviceBuffer[1] == 0x01){
				sprintf((char *)txBuffer,"Data Transfer success\r\n");
				HAL_UART_Transmit(userHuart, txBuffer, sizeof(txBuffer),10);
				networkJoinStatus = NETWORK_JOIN_OK;
			}else{
				sprintf((char *)txBuffer,"Data Transfer failed\r\n");
				HAL_UART_Transmit(userHuart, txBuffer, sizeof(txBuffer),10);
				networkJoinStatus = NETWORK_JOIN_ERROR;
				//The device failed to send the data. Throw an error to display a message on UART and send a error message to the server
				//Of course, if the device is not connected, the error message won't be sent ^^
				ThrowError(BTERROR_SENDING_TIMEOUT);
			}
			rxDeviceState = RCV_HEAD;
			HAL_UART_Receive_IT(deviceHuart,deviceUartRxBuffer,1);
			break;
		case RCV_DATA_LENGTH : //Receiving the body length of the data message
			rcvDataLength = *deviceUartRxBuffer;
			HAL_UART_Receive_IT(deviceHuart, rxDeviceBuffer, rcvDataLength);
			rxDeviceState = RCV_DATA;
			break;
		case RCV_DATA : // Receiving the body of a data message
			if((HAL_GetTick() - lastCommandTick) >= 5000){//At least 5 seconds between two events processing
				deviceCommandReceivedHandler(rxDeviceBuffer, rcvDataLength);
				rcvDataLength = 0;
				rxDeviceState = RCV_HEAD;
				lastCommandTick = HAL_GetTick();
			}
			HAL_UART_Receive_IT(deviceHuart,deviceUartRxBuffer,1); //Enable next message's head reception
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
 *
 * This function will initialize the library :
 * 		=> set the UART peripherals to be used : userUart and deviceUart
 * 		=> set the buffer's address to be used to analyze received characters from user input : userInputBuffer
 * 		=> set the user input buffer reset routine : resetInputBufferHandler
 * 		=> set the function to handle commands/data received by the node from the gateway : deviceCommandReceivedFunction
 * 		=> set the function to perform the data sensing and formatting (JSON format) : getSensorDataFunction
 * 		=> set the function to be called when the sensor's data has been sent : dataSentCallback
 * 		=> set the maximum length of the data payload : sensorDataMaxLength
 */
void BTDevice_init(BTDevice_InitTypeDef *BTDevice_InitStruct){
	uint8_t errorDetected = 0; //Assume there is no error

	if(BTDevice_InitStruct->userHuart == NULL) errorDetected = 1;
	if(BTDevice_InitStruct->deviceHuart == NULL) errorDetected = 1;
	if(BTDevice_InitStruct->deviceCommandReceivedHandler == NULL) errorDetected = 1;
	if(BTDevice_InitStruct->getSensorDataFunction == NULL) errorDetected = 1;
	if(BTDevice_InitStruct->signalEventFunction == NULL) errorDetected = 1;


	setUserHuart(BTDevice_InitStruct->userHuart);
	setDeviceHuart(BTDevice_InitStruct->deviceHuart);
	setDeviceCommandReceivedHandler(BTDevice_InitStruct->deviceCommandReceivedHandler);
	setSensorDataFunction(BTDevice_InitStruct->getSensorDataFunction);
	setSignalEventFunction(BTDevice_InitStruct->signalEventFunction);

	if(errorDetected == 1)
		ThrowError(BTERROR_INIT_STRUCTURE);
}

/**
 *
 * This function should be called by BTDevice_mainLoop following a received character  in the userUart
 *
 * There are three "modes" that handle three different cases :
 *
 * 		COMMAND_MODE : user is typing a command
 * 		DATA INPUT : user is entering data to send to the gateway
 * 		TIMER SETTINGS : user is entering a new value for the timer period
 *
 * 	DATA INPUT and TIMER SETTINGS mode require the user to type a special word in order to signal the end of it's input : " end" (notice the space)
 *
 * 	When in COMMAND MODE, the user input is compared to a set of reference strings (@commandArray)
 * 	If a command is recognized, the appropriate handler function is executed (or the mode is changed to fit the new context)
 */
void readInputBuffer(){
	uint8_t commandIterator;
	uint8_t commandRecognized = 0;
	uint16_t len = strlen((const char *)userInputBuffer);
	HAL_UART_Transmit(userHuart, &userInputBuffer[len-1],sizeof(uint8_t),10);
	switch(getUserUartMode()){
	case  COMMAND_MODE:
		//Before looking for a command, check for 'rs' wich is a special command to reset the userInputBuffer
		if(checkForInputBufferReset() == 1){
			HAL_UART_Transmit(userHuart,(uint8_t *)"\r\n",2,10);
			rsHandler();
			resetInputBuffer();
		}else{
			//Compare userInputBuffer to the reference strings (@commandArray)
			for(commandIterator = 0; commandIterator < COMMAND_NB; commandIterator++){
				if(strcmp((const char *)commandArray[commandIterator],(const char *)userInputBuffer) == 0){
					commandRecognized = 1;
					break;
				}
			}
		}
		//If a command is recognized, it will call the appropriate handler. This require a bit more details.
		//There are two arrays of same length : one for the reference strings (@commandArray) and one for the corresponding handler
		//function (@commandFunctionArray). They work by pair and have the same index in the array.
		if(commandRecognized == 1){
			HAL_UART_Transmit(userHuart,(uint8_t *)"\r\n",2,10);
			(commandFunctionArray[commandIterator])();
			resetInputBuffer();
		}
		break;
	case DATA_INPUT:
		if(getDataFromUser() == RESET_BUFFER){
			resetInputBuffer();
		}
		break;
	case TIMER_SETTINGS:
		if(setTimerPeriod() == RESET_BUFFER){
			resetInputBuffer();
		}
		break;
	}
}


/**
 * This function send a data buffer via the BTDevice.
 * A copy of the data is displayed via the userUart.
 * When automode is turned ON, this function is called periodically.
 *
 * WARNING : Please be aware that dataBuffer must have been dynamically allocated before being passed in parameters. It is free'd deeper in
 * the library
 *
 * @param [in] dataBuffer the data so be sent
 * @param [in] the maximum size of the data buffer
 * @pre dataBuffer must be dynamically allocated (malloc). It is free'd (free) deeper in the library.
 */
void BTDevice_sendData(uint8_t *dataBuffer, uint16_t dataBufferLength){

	//Analyze the sensor data in case of format error, device unresponsive or payload overflow
	checkSensorData(dataBuffer);

	uint8_t txBuffer[64];
	memset(txBuffer,0, sizeof(txBuffer));
	if(getAutoModeStatus() == AUTOMODE_ON){
		sprintf((char *)txBuffer,"AutoMode is ON\r\n");
		HAL_UART_Transmit(userHuart, txBuffer,sizeof(txBuffer),10);
	}else{
		sprintf((char *)txBuffer,"AutoMode is OFF\r\n");
		HAL_UART_Transmit(userHuart, txBuffer,sizeof(txBuffer),10);
	}

	sendDataToDevice(dataBuffer,dataBufferLength);
	free(dataBuffer);
}


/**
 * This function should be called from the BTDevice_mainLoop following a timer UP interupt (supposed to happen every second) or a flush command
 *
 * When user ask for a flush command, the number of seconds waited since last transfer is reset and 1 is returned
 *
 * Otherwise, if AutoMode status is ON, it will compare the number of seconds waited since last data transfer to the timer period value
 * set during Init process or changed using the appropriate command. If enough seconds have been waited, return 1 to trigger the data sending
 *
 * @retval 1 to trigger the sending of data, 0 to skip the sending for this call
 */
uint8_t timerReadyToSend(){
	if(flushSensorDataFlag == 1){
		flushSensorDataFlag = 0;
		resetPeriodCounter();//Reset the number of seconds waited since last data transfer
		return 1;
	}else if(getAutoModeStatus() == AUTOMODE_ON){
		incrementPeriodCounter();
		if(getPeriodCounter() >= getTimerPeriod()){
			resetPeriodCounter();
			return 1;
		}
	}
	return 0;
}

/**
 * This function should be called from the Uart Rx Complete callback following the reception of one character on the userUart
 * It will raise a flag to signal the mainLoop that it should analyze the received data to check for a command
 *
 * The current tick is saved in lastInteruptTick and will be used to wait a short time (50ms) before starting to analyze the userInputBuffer
 * This short time allow reception of several characters in high speed since the CPU is not busy analyzing what was received
 *
 * @param [in] *userUartTxBuffer A single byte buffer that is used to receive characters one by one from input in interput mode
 */
void BTDevice_userUartCallback(uint8_t *userUartRxBuffer)
{
	storeNewValueIntoInputBuffer(*userUartRxBuffer);
	userUartCallbackFlag = 1;
	HAL_UART_Receive_IT(userHuart,userUartRxBuffer, 1);
	lastInterruptTick = HAL_GetTick();
}


/**
 * This function should be called from the UART Rx Complete callback following the reception of one character on the deviceUart
 * It will raise a flag to signal to the mainLoop that it should analyze and process the received data
 *
 * If the received byte is the head of a message, the reception for the rest of the message will be allwed by the library to match the
 * message's length
 *
 * @param [in] *deviceUartRxBuffer A single byte buffer to receive characters one by one from input in interupt mode
 */
void BTDevice_deviceUartCallback(uint8_t *deviceUartRxBuffer)
{
	deviceUartCallbackFlag = 1;
	localDeviceUartRxBuffer = deviceUartRxBuffer;
}


/**
 * This function should be called from the TIM Period Ellapsed calback following an interupt (which should occur every second if well configured)
 *
 * It will raise a flag to signal to the mainLoop to check wheter or not it should send a new set of data. When AutoMode is on, data will be
 * sent periodically, mainLoop will check how much time was waited since last data transfer and compare it to the reference timer period value.
 * If enough time has been waited, data will be sent.
 *
 */
void BTDevice_timerCallback()
{
	timerCallbackFlag = 1;
}

/**
 * This function will initialize the library according to the values specified in the init structure
 *
 * It will loop untill the gateway network is joined.
 *
 * When the intialisation process fail, it will call the initFailed function.
 * If the process is successful, it will call the initSucces function.
 */
BTDevice_Status BTDevice_initLoop(BTDevice_AutoInitTypeDef defaultValues){
	if(defaultValues.autoModeStatus == 0) ThrowError(BTERROR_AUTOINIT_STRUCTURE);
	if(defaultValues.timerPeriodValue == 0) ThrowError(BTERROR_AUTOINIT_STRUCTURE);

	if(deviceUartCallbackFlag){
		deviceUartCallbackFlag = 0;
		deviceUartCallback(localDeviceUartRxBuffer);
	}

	if(timerCallbackFlag)
		timerCallbackFlag = 0;

	if((HAL_GetTick() - lastInterruptTick) > 25){
		if(userUartCallbackFlag){
			userUartCallbackFlag = 0;
			uint8_t txBuffer[128];
			memset(txBuffer,0,sizeof(txBuffer));
			sprintf((char *)txBuffer, "\r\n >>WARNING<< Device Initialization process running. Wait till end before typing commands\r\n");
			HAL_UART_Transmit(userHuart, txBuffer, sizeof(txBuffer), 10);
			resetInputBuffer();
		}
	}

	return autoInitWithDefaultValues(defaultValues);
}


/**
 * This function is the mainLoop of the library. It should be called indefinitely from user space
 *
 * It will check for the peripherals flags (UART IT or TIM IT) evoqued above and will call the appropriate functions depending on the situation.
 *
 */
void BTDevice_mainLoop()
{

	static uint32_t lastSignalTick = 0;
	//A character was received on the deviceUart
	if(deviceUartCallbackFlag){
		deviceUartCallbackFlag = 0;
		deviceUartCallback(localDeviceUartRxBuffer);
	}

	//A TIM PeriodEllapsed interupt has occured
	if(timerCallbackFlag){
		timerCallbackFlag = 0;
		if(timerReadyToSend()){
			uint8_t *tmp = getSensorData(); //Get the data (WARNING ! getSensorData must return a dynamically allocated buffer
			BTDevice_sendData(tmp, SENSOR_DATA_MAX_LENGTH);  //Send the data and free the buffer

			//I disabled LED signal when sending data so that LED blink only in case of problems
			//signalEventFunction(NULL); //Execute Callback once the data has been sent
		}
	}

	//A character was received on the userUart
	//Wait at least 25ms without IT to begin process userInput
	if((HAL_GetTick() - lastInterruptTick) > 25){
		if(userUartCallbackFlag){
			userUartCallbackFlag = 0;
			readInputBuffer();
		}
	}

	//When the device is disconnected, every 5 seconds, blink the LED and try to reconnect
	if(getNetworkJoinStatus() == NETWORK_JOIN_ERROR && ((HAL_GetTick() - lastSignalTick) > 10000 || lastSignalTick == 0)){
		signalEventFunction(NULL);
		uint8_t txBuffer[128] = "";
		sprintf((char *)txBuffer,"Device disconnected. Trying to reconnect\r\n");
		HAL_UART_Transmit(userHuart, txBuffer,sizeof(txBuffer),10);
		networkJoinHandler();
		lastSignalTick = HAL_GetTick();
	}
}

/*****************************
 * ERROR HANDLING
 *****************************/
/**
 * Handle incoming errors. An error message will be displayed on the userUart if possible.
 * The error message will be forwarded to the server if possible
 *
 * @param [in] errorCode The error code corresponding to the error
 * @pre the error code must have been declared in BTDevice_Error enumeration
 */
static void ThrowError(BTDevice_Error errorCode){

	//Array of flags for each error case. Set flag to 1 to prevent another error message of the same type.
	static uint8_t errorDisabledArray[BTERROR_NUMBER_OF_ERRORS] = {0};

	uint8_t errorMessage[256];
	uint16_t errorMessageLength = 0;
	memset(errorMessage,0,sizeof(errorMessage));

	//System tick at the time of the error. Save the value to keep track of the time.
	static uint32_t sendingTimeoutErrorTick = 0; //When sending error occur, it will be re-enabled after a short while
	static uint32_t sensorUnresponsiveErrorTick = 0;


	switch(errorCode)
	{

	/*
	 * The device received bad signal information (RSSI = 0 and SNR = 0)
	 */
	case BTERROR_BAD_SIGNAL :
		if(errorDisabledArray[BTERROR_BAD_SIGNAL] == 0){//Check that the error was not disabled

			errorMessageLength = sprintf((char *)errorMessage,
					"RF signal is abnormally poor: RSSI = 0 and SNR = 0");
			forwardErrorToServer(errorMessage,errorMessageLength);
		}
		break;

		/*
		 * AutoInit procedure was not succesful after five cycles
		 */
	case BTERROR_AUTOINIT_TIMEOUT :
		if(errorDisabledArray[BTERROR_AUTOINIT_TIMEOUT] == 0){//Check that the error was not disabled

			errorMessageLength = sprintf((char *)errorMessage,
					"Device taking too long to join netwok");
			forwardErrorToServer(errorMessage,errorMessageLength);

			//Prevent sending this error message again (Can't repeat AutoInit procedure anyway)
			errorDisabledArray[BTERROR_AUTOINIT_TIMEOUT] = 1;
		}
		break;


		/*
		 * The sensor data function "getSensorData()" did not reply a formatted data buffer (JSON format)
		 */
	case BTERROR_SENSOR_DATA_FORMAT :
		if(errorDisabledArray[BTERROR_SENSOR_DATA_FORMAT] == 0){//Check that the error was not disabled

			errorMessageLength = sprintf((char *)errorMessage,
					"Sensor data does not match JSON format.");
			forwardErrorToServer(errorMessage,errorMessageLength);

			//Prevent sending this error message again (format won't change itself, no point repeating the message)
			errorDisabledArray[BTERROR_SENSOR_DATA_FORMAT] = 1;
		}
		break;


		/*
		 * The sensor data is wrong (eg = 0), should probably check wiring and peripheral init.
		 */
	case BTERROR_SENSOR_UNRESPONSIVE :
		if(errorDisabledArray[BTERROR_SENSOR_UNRESPONSIVE] == 0){//Check that the error was not disabled
			errorMessageLength = sprintf((char *)errorMessage,
					"Sensor data was null. Sensor unresponsive");
			forwardErrorToServer(errorMessage,errorMessageLength);

			//Prevent sending this error message again. It will be enabled again after some time (see below)
			errorDisabledArray[BTERROR_SENSOR_UNRESPONSIVE] = 1;

			//Save the current Tick to be able to re-enable the error after some time
			sensorUnresponsiveErrorTick = HAL_GetTick();
		}
		//After some time, re-enable the error to check whether the problem was solved of not
		if((HAL_GetTick() - sensorUnresponsiveErrorTick) > 10000) errorDisabledArray[BTERROR_SENSOR_UNRESPONSIVE] = 0;

		break;

		/*
		 * The sensor data was not sent by the device (received a Data Transfer Fail message from the LoRa Module)
		 */
	case BTERROR_SENDING_TIMEOUT : //Test OK
		if(errorDisabledArray[BTERROR_SENDING_TIMEOUT] == 0){//Check that the error was not disabled
			errorMessageLength = sprintf((char *)errorMessage,
					"The device could not send the data in time.");
			forwardErrorToServer(errorMessage,errorMessageLength);

			//Prevent sending this error message again. It will be enabled again after some time (see below)
			errorDisabledArray[BTERROR_SENDING_TIMEOUT] = 1;

			// Save the current tick to be able to re-enable the error after some time
			sendingTimeoutErrorTick = HAL_GetTick();
		}

		//After some time, re-enable the error to check whether the problem was solved of not
		if((HAL_GetTick() - sendingTimeoutErrorTick) > 10000) errorDisabledArray[BTERROR_SENDING_TIMEOUT] = 0;
		break;


		/*
		 * The init structure was not filled properly
		 */
	case BTERROR_INIT_STRUCTURE:
		if(errorDisabledArray[BTERROR_INIT_STRUCTURE] == 0){//Check that the error was not disabled
			errorMessageLength = sprintf((char *)errorMessage,
					"The init structure not properly filled.");
			forwardErrorToServer(errorMessage,errorMessageLength);

			//Prevent sending this error message again (structure won't fill itself, no point repeating the message)
			errorDisabledArray[BTERROR_INIT_STRUCTURE] = 1;
		}
		break;


		/*
		 * The AutoInit structure was not filled properly
		 */
	case BTERROR_AUTOINIT_STRUCTURE :
		if(errorDisabledArray[BTERROR_AUTOINIT_STRUCTURE] == 0){//Check that the error was not disabled
			errorMessageLength = sprintf((char *)errorMessage,
					"The auto-init structure not properly filled.");
			forwardErrorToServer(errorMessage,errorMessageLength);

			//Prevent sending this error message again (structure won't fill itself, no point repeating the message)
			errorDisabledArray[BTERROR_AUTOINIT_STRUCTURE] = 1;
		}
		break;

		/*
		 * Too many bytes to send in the payload
		 */
	case BTERROR_DEVICE_PAYLOAD_OVFL :
		if(errorDisabledArray[BTERROR_DEVICE_PAYLOAD_OVFL] == 0){//Check that the error was not disabled
			errorMessageLength = sprintf((char *)errorMessage,
					"Data payload overflow.");
			forwardErrorToServer(errorMessage,errorMessageLength);

			//Prevent sending this error message again (no point repeating the message)
			errorDisabledArray[BTERROR_DEVICE_PAYLOAD_OVFL] = 1;
		}
		break;


	case BTERROR_NUMBER_OF_ERRORS:
		//No need to do anything here.
		//It's just to prevent warning for not handling all enum cases
		break;
	}
}


static void forwardErrorToServer(uint8_t *errorMessage, uint16_t errorMessageLength){
	//Create a buffer fit for the error message (dynamically allocated, it is free'd by BTDevice_sendData)
	uint8_t *txBuffer = malloc((sizeof(uint8_t)*errorMessageLength)+sizeof("{\"ERROR\":\"\"}"));

	//Format the error message in following format : {"ERROR","ERROR_MESSAGE"}
	sprintf((char *)txBuffer,"{\"ERROR\":\"%s\"}",errorMessage);

	//Send the message to the server and free the buffer
	BTDevice_sendData(txBuffer, strlen((const char *)txBuffer));
}

