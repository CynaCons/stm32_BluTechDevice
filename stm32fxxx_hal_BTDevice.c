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
static void setDeviceCommandReceivedHandler(void (*fPtr)(uint8_t *dataBuffer, uint16_t dataLength));
static void setDataSentCallback(void (*fPtr)(void));
static void setSensorDataFunction(uint8_t* (*fPtr)(void));
static void setSensorDataMaxLength(uint16_t maxLength);
static void setUserHuart(UART_HandleTypeDef *huartx);
static void setUserInputBuffer(uint8_t *ptrBuffer);
static void setResetInputBufferHandler(void (*fPtr)());
static void sendDataToDevice(uint8_t *dataBuffer, uint16_t dataLength);
static void startAutoMode();
static void stopAutoMode();
static uint8_t setTimerPeriod();
static void resetPeriodCounter(void);
static uint8_t getAutoModeStatus(void);
static void incrementPeriodCounter(void);
static uint32_t getPeriodCounter(void);
static uint32_t getTimerPeriod(void);
static void resetDeviceInputBuffer(void);


//Handlers for each command
static void autoModeOnHandler(void);
static void autoModeOffHandler(void);
static uint8_t getDataFromUser(void);
static void getNetworkStatusHandler(void);
static void getTimerPeriodHandler(void);
static void getSensorValueHandler(void);
static void menuHandler(void);
static void networkJoinHandler(void);
static void rfSignalCheckHandler(void);
static void rsHandler(void);
static void sendDataHandler(void);
static void sendSampleDataHandler(void);
static void setTimerPeriodHandler(void);
static void flushSensorDataHandler(void);
static void getAutoModeStatusHandler(void);
static void deviceStatusHandler(void);



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
	GET_NETWORK_STATUS, /*<! Returns the current state of network (joined or not)*/
	FLUSH_SENSOR_DATA, /*<! Force the sending of sensor data and reset the period counter */
	GET_AUTOMODE_STATUS, //TODO COMMENT
	DEVICE_STATUS,
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
		"get sensor value",
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
//TODO Maybe add the proper return type to all of the functions ?

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

void (*resetInputBufferHandler)();/*!< Function pointer to the function used to reset the user input buffer */

void (*deviceCommandReceivedHandler)(uint8_t *dataBuffer, uint16_t dataLength); /*!< Function pointer the function
that will process the received data/command (received by the node from the gateway)*/

static uint8_t flushSensorDataFlag = 0;

static uint32_t lastCommandTick = 0; //SysTick value (HAL_GetTick) corresponding to the last command/data received processing. Anti-Spam.

static uint8_t networkJoinStatus = NETWORK_JOIN_ERROR; /*<! Current state of the network : NETWORK_JOIN_OK or NETWORK_JOIN_ERROR */

static uint8_t rxDeviceBuffer[DEVICE_DATA_MAX_LENGTH]; /*!< Device Input buffer */

static uint8_t rxDeviceState = RCV_HEAD; /*!< Current state for the deviceUart. See enum definition */

static volatile uint16_t rcvDataLength = 0; /*!< Global container to store how much data will be received */

static uint32_t timerPeriod = 5; /*!<Default value for period between two transfer. Use menu to change or AutoInit procedure */

static uint8_t *userInputBuffer = NULL; /*!< Adress of the buffer that is read to analyse user's input */

static UART_HandleTypeDef *userHuart; /*!< This uart is used to communicate with the user through serial cable */

static uint8_t userUartMode = COMMAND_MODE;/*!< Current state for the userUart. See enum definition*/

static uint8_t * savedDataBuffer = NULL; /*!< Dynamicaly allocated(strdup) buffer containing the last data sent */ 

//TODO
//Flags
static uint8_t userUartCallbackFlag = 0;

static uint8_t deviceUartCallbackFlag = 0;

static uint8_t timerCallbackFlag = 0;

static uint8_t *localDeviceUartRxBuffer = NULL;

static uint8_t* (*getSensorData)(void);

static uint16_t sensorDataMaxLength = 0;

static void (*dataSentCallback)(void);

static uint32_t lastInterruptTick = 0;

static uint8_t rfSignalCheckCopy[4] = ""; // length = 4. 3 bytes for signal info and one byte for terminating '\0'

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
			(uint8_t *)"= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = \r\n",
			(uint8_t *)"			                APPLICATION MENU		                       	\r\n",
			(uint8_t *)"	                                                                        \r\n",
			(uint8_t *)"\r\n",
			(uint8_t *)"+++   GENERAL CONTROL\r\n",
			(uint8_t *)"    --> menu                : display this menu\r\n",
			(uint8_t *)"    --> device status       : display a status report for this device\r\n",
			(uint8_t *)"    --> rs                  : reset the input buffer (in case of typing mistake)\r\n",
			(uint8_t *)"    --> get sensor value    : display the last sensor value\r\n",
			(uint8_t *)"    --> set automode on     : start sending data periodically\r\n",
			(uint8_t *)"    --> set automode off    : stop sending data periodically\r\n",
			(uint8_t *)"    --> get automode status : display the automode status (ON or OFF)\r\n",
			(uint8_t *)"    --> send data           : input some ascii data that will be sent to the gateway\r\n",
			(uint8_t *)"    --> flush sensor data   : instantly send sensor data and reset the timer period\r\n",
			(uint8_t *)"\r\n",
			(uint8_t *)"+++   BLUTECH DEVICE CONTROL\r\n",
			(uint8_t *)"    --> rf signal check     : perform a signal check\r\n",
			(uint8_t *)"    --> network join        : join the gateway network\r\n",
			(uint8_t *)"    --> send sample data    : send a sample of data for testing\r\n",
			(uint8_t *)"    --> get network status  : display the network status (CONNECTED or not)\r\n",
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
 * Set the function pointer for the resetInputBuffer handler.
 * 		# The user input buffer (declared and filled in main, it's address is given to the library during Init so
 * 			that the library can analyze it
 * 		# The function to reset this buffer should be written in main.c by the user and the function's pointer
 * 		should be passed to the library during init so that the buffer could be cleaned FROM the library.
 *		# The function can then be called using the function pointer "resetInputBufferHandler();"
 *
 *	@param *fptr function pointer to the routine that should perform the reset
 */
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


/*
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

static void setSensorDataFunction(uint8_t* (*fPtr)(void))
{ //TODO COMMENT
	getSensorData = fPtr;
}

static void setSensorDataMaxLength(uint16_t maxLength)
{
	sensorDataMaxLength = maxLength;
}

static void setDataSentCallback(void (*fPtr)(void))
{
	dataSentCallback = fPtr;
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
	sprintf((char *)txBuffer,"\r\nAutoMode Started !\r\n");
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
 * Code and functions titles are self-explanatory. No need for comments.
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

static void flushSensorDataHandler(void){
	uint8_t txBuffer[128];
	memset(txBuffer,0, sizeof(txBuffer));
	flushSensorDataFlag = 1;
	sprintf((char *)txBuffer,"\r\nFlushing the sensor data...\r\n");
	HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
}

static void getAutoModeStatusHandler(void){
	uint8_t txBuffer[128];
	memset(txBuffer,0,sizeof(txBuffer));
	if(getAutoModeStatus() == AUTOMODE_OFF){
		sprintf((char *)txBuffer, "\r\nAutoMode is OFF\r\n");
		HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
	}else{
		sprintf((char *)txBuffer, "\r\nAutoMode is ON\r\n");
		HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
	}
}

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
	if(rfSignalCheckCopy[0] != '\0')//TODO FIX THIS PLZ
		sprintf((char *)txBuffer,"Signal info : RSSI : %x%x ; SNR : %x\r\n",rfSignalCheckCopy[0],rfSignalCheckCopy[1],rfSignalCheckCopy[2]);
	else
		sprintf((char *)txBuffer,"Signal info : no signal info available. Do \"rf signal check \" first\r\n");
	HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
	getSensorValueHandler();
	memset(txBuffer,0,sizeof(txBuffer));
	sprintf((char *)txBuffer,"\r\n . . . . . . . .Device status report END. . . . . . . .\r\n");
	HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
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

static void getNetworkStatusHandler(void){
	uint8_t txBuffer[128];
	memset(txBuffer,0,sizeof(txBuffer));
	if(getNetworkJoinStatus() == NETWORK_JOIN_OK){
		sprintf((char *)txBuffer,"\r\nCurrent network status : NETWORK JOIN OK\r\n");
		HAL_UART_Transmit(userHuart, txBuffer, sizeof(txBuffer),10);
	}else{
		sprintf((char *)txBuffer, "\r\nCurrent network status : NETWORK JOIN ERROR\r\n");
		HAL_UART_Transmit(userHuart, txBuffer, sizeof(txBuffer),10);
	}

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


/*
 * This function should be used and called from user main when a device must be initialized on startup without further configuration.
 *
 * This function will set the timer period value, the automode status and will perform a network join
 * Then it waits 4 seconds in blocking mode and check whether or not the network join was successful
 * If network join was successful it returns BTDevice_OK, otherwise it returns BTDevice_ERROR
 *
 * See formatted commenting in the header file
 */
BTDevice_Status BTDevice_autoInitWithDefaultValues(BTDevice_AutoInitTypeDef defaultValues){
	static uint32_t startTick = 0;
	static uint8_t txBuffer[128];


	if((HAL_GetTick() - startTick) >= 10000 || startTick == 0){
		memset(txBuffer,0,sizeof(txBuffer));
		sprintf((char *)txBuffer,"\r\n . . . . . . . .Launching AutoInit Procedure. . . . . . . .\r\n");
		HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
		startTick = HAL_GetTick();
		timerPeriod = defaultValues.timerPeriodValue;
		if(defaultValues.autoModeStatus == AUTOMODE_OFF)
			autoModeOffHandler();
		else
			autoModeOnHandler();

		networkJoinHandler();
		//Wait 10 second to receive the network join confirmation

	}
	if(getNetworkJoinStatus() == NETWORK_JOIN_OK){
		memset(txBuffer,0,sizeof(txBuffer));
		sprintf((char *)txBuffer,"\r\n . . . . . . . .AutoInit Procedure Succes. . . . . . . .\r\n");
		HAL_UART_Transmit(userHuart,txBuffer,sizeof(txBuffer),10);
		return BTDevice_OK;
	}
	return BTDevice_ERROR;
}


/**
 * This function handle the communication from the BTDevice to the MCU
 * First, we receive the "head" (the first byte). Depending on the head value,
 * we start IT reception process for the rest of the message.
 * Once the message is completely received, we then display it to the user
 * through the userUart.
 */
//TODO : Sometimes when trying to receive the body of a command we probably receive some other commands at the same time.
//TODO : The right solution  would probably be to store all the incoming bytes on IT and then read what we received in a queued manner
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
			}else{
				sprintf((char *)txBuffer,"Data Transfer failed\r\n");
				HAL_UART_Transmit(userHuart, txBuffer, sizeof(txBuffer),10);
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
 */
void BTDevice_init(BTDevice_InitTypeDef *BTDevice_InitStruct){
	setUserHuart(BTDevice_InitStruct->userHuart);
	setDeviceHuart(BTDevice_InitStruct->deviceHuart);
	setUserInputBuffer(BTDevice_InitStruct->userInputBuffer);
	setResetInputBufferHandler(BTDevice_InitStruct->resetInputBufferHandler);
	setDeviceCommandReceivedHandler(BTDevice_InitStruct->deviceCommandReceivedHandler);
	setSensorDataFunction(BTDevice_InitStruct->getSensorDataFunction);
	setSensorDataMaxLength(BTDevice_InitStruct->sensorDataMaxLength);
	setDataSentCallback(BTDevice_InitStruct->dataSentCallback);
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
void readInputBuffer(){
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

//TODO Move the RXBuffer inside the library
void BTDevice_userUartCallback(uint8_t *userUartRxBuffer)
{
	userUartCallbackFlag = 1;
	HAL_UART_Receive_IT(userHuart,userUartRxBuffer, 1);
	lastInterruptTick = HAL_GetTick();
}

void BTDevice_deviceUartCallback(uint8_t *deviceUartRxBuffer)
{
	deviceUartCallbackFlag = 1;
	localDeviceUartRxBuffer = deviceUartRxBuffer;
}

void BTDevice_timerCallback()
{
	timerCallbackFlag = 1;
}


void BTDevice_initLoop(){
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
			resetInputBufferHandler();
		}
	}
}

void BTDevice_mainLoop()
{
	if(deviceUartCallbackFlag){
		deviceUartCallbackFlag = 0;
		deviceUartCallback(localDeviceUartRxBuffer);
	}

	if(timerCallbackFlag){
		timerCallbackFlag = 0;
		if(timerReadyToSend()){
			uint8_t *tmp = getSensorData(); //fPtr must be passed during init
			BTDevice_sendData(tmp, sensorDataMaxLength); //alue must be passed during init
			free(tmp);
			dataSentCallback();//Implement this to INIT procedure
		}
	}
	//Wait at least 25ms without IT to begin process userInput
	if((HAL_GetTick() - lastInterruptTick) > 25){
		if(userUartCallbackFlag){
			userUartCallbackFlag = 0;
			readInputBuffer();
		}

	}
}



