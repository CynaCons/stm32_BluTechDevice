/*
 * stm32fxxx_hal_BTDevice.c
 *
 *  Created on: Jul 6, 2016
 *      Author: constantin.chabirand@gmail.com
 */


#include <stdint.h>
#include <string.h>
#include <stdlib.h>

//HELLO WORLD



#if defined(STM32F103RCTx) || defined(STM32F103xx) || defined(STM32F103)|| defined(STM32F103xE)
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_uart.h"
#endif

#if defined(STM32F4DISCOVERY)|| defined(STM32F407VGTx)|| defined(STM32F407xx)
#include "stm32f4xx_hal.h"
#include "stm32f4xx_hal_uart.h"
#endif

#include "stm32fxxx_hal_BTDevice.h"


#define NB_MENU_LINES 21


static uint8_t checkForInputBufferReset(void);
static void displayMenu();
static void setUserUartInTimerSettingsMode(void);
static void setUserUartInDataInputMode(void);
static void setUserUartInCommandMode(void);
static void setDeviceHuart(UART_HandleTypeDef *huartx);
static void setUserHuart(UART_HandleTypeDef *huartx);
static void setUserInputBuffer(uint8_t *ptrBuffer);
static void sendDataToDevice(uint8_t *dataBuffer, uint16_t dataLength);
static void startAutoMode();
static void stopAutoMode();
static uint8_t setTimerPeriod();



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

/**
 * Unique number for each command that the user can send
 */
typedef enum {
	MENU, /*!< Display the UI menu */
	AUTO_ON, /*!< Start Auto Mode */
	AUTO_OFF, /*!< Stop Auto Mode */
	SEND_DATA, /*!< Send data to the device (also on the user uart */
	DISPLAY_SENSOR_VALUE, /*!< Display the current sensor value */
	RFSIGNALCHECK, /*!< Send command to the device to perform a signal check */
	NETWORKJOIN, /*!< Send command to the device to join the BT LoRa network */
	SENDSAMPLEDATA, /*!< Send data sample to the device */
	SET_TIMER_PERIOD, /*!< Set the timer period */
	GET_TIMER_PERIOD, /*!< Returns the current timer period*/
	COMMAND_NB /*!< Total number of commands that the user can send */
} commandID;


/**
 * The specific string that the user has to write in order to perform the equivalent command. See above for comments
 */
uint8_t commandArray[COMMAND_NB][32] = {
		"menu",
		"set automode on",
		"set automode off",
		"send data",
		"get sensor value"
		"rf signal check",
		"network join",
		"send sample data",
		"set timer period",
		"get timer period"
};


/**
 * Function pointers to each handler function which will do the action requested by the user
 */
//Reminder : void (*)(void) fp -- Use the preceding syntax to create an array of function pointers that returns void
void (*commandFunctionArray[COMMAND_NB])(void) =  {
		&menuHandler,
		&autoModeOnHandler,
		&autoModeOffHandler,
		&sendDataHandler,
		&getSensorValueHandler,//TODO ADD THIS PLZ
		&rfSignalCheckHandler,
		&networkJoinHandler,
		&sendSampleDataHandler,
		&setTimerPeriodHandler,
		&getTimerPeriodHandler
};


static uint8_t *userInputBuffer = NULL;
static uint32_t timerPeriod = 5; /*!< This variable is set using BTDevice_setTimerPeriod */
static uint32_t periodCounter = 0; /*!< Number of seconds to wait between two data transfer */



static UART_HandleTypeDef userHuart; /*!< This uart is used to communicate with the user through serial cable */
static UART_HandleTypeDef deviceHuart; /*!< This uart is used to send/receive commands from the BT device using BT's UART commands */


static uint8_t autoModeStatus = AUTOMODE_OFF; /*!< Definition in header file. When AutoMode is on data will be send every timer period */
static uint8_t userUartMode = COMMAND_MODE;/*!< Definition in header file. Use getter/setter to modify the value  */



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
	uint8_t menuContent[NB_MENU_LINES][128] = {
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
	};

	//Display the menu
	uint8_t menuIterator = 0;
	for(menuIterator = 0; menuIterator < NB_MENU_LINES; menuIterator++){
		HAL_UART_Transmit(&userHuart,(uint8_t *)menuContent[menuIterator], strlen((const char *)menuContent[menuIterator]),10);
	}
}


/**
 * Set user uart in timer settings mode
 */
static void setUserUartInTimerSettingsMode(){
	userUartMode = TIMER_SETTINGS;
	uint8_t buffer[128] = "\r\nUser Uart now in timer settings mode. Commands won't be recognized\r\n";
	HAL_UART_Transmit(&userHuart,buffer, sizeof(buffer),10);
	memset(buffer,0,sizeof(buffer));
	sprintf((char *)buffer,"Pease type the new timer period value followed by end(max 65000)\r\n Like this : 25 end\r\n");
	HAL_UART_Transmit(&userHuart,buffer,sizeof(buffer),10);
	memset(buffer, 0, sizeof(buffer));
	sprintf((char *)buffer, "The previous command '25 end' means 25 seconds between two data transfer\r\n");
	HAL_UART_Transmit(&userHuart,buffer,sizeof(buffer),10);
}

/**
 * Set user uart in data input mode
 */
static void setUserUartInDataInputMode(){
	userUartMode = DATA_INPUT;
	uint8_t buffer[128] = "\r\nUser Uart now in data input mode. Commands won't be recognized\r\n";
	HAL_UART_Transmit(&userHuart,buffer, sizeof(buffer),10);
	memset(buffer,0,sizeof(buffer));
	sprintf((char *)buffer,"Pease type the data to send followed by end(max 64 bytes)\r\n Like this : myData end\r\n");
	HAL_UART_Transmit(&userHuart,buffer,sizeof(buffer),10);
}


/**
 * Set user uart in command mode
 */
static void setUserUartInCommandMode(){
	userUartMode = COMMAND_MODE;
	uint8_t buffer[] = "\r\nUser Uart now in command mode. Commands will be recognized\r\n";
	HAL_UART_Transmit(&userHuart,buffer, sizeof(buffer),10);
	displayMenu();
}


/**
 * Set the uart to be used as user uart for the BTDevice library. This user uart is then used to display the UI and configure the board.
 * @param [in] huartx UART_HandleTypeDef's address of the uart handler to use.
 * @pre The uartx must have been initialized before in IT mode.
 */
void setUserHuart(UART_HandleTypeDef *huartx){
	userHuart = *huartx;
}


/**
 * Set the uart to be used as device uart for the BTDevice library. This device uart is then used to send commands to the BluTech device.
 * @param [in] huartx UART_HandleTypeDef's address of the uart handler to use.
 * @pre The huartx must have bee initalized before.
 */
void setDeviceHuart(UART_HandleTypeDef *huartx){
	deviceHuart = *huartx;
}


/**
 * Set the user input buffer's address to be check to recognize user's commands and set it's length.
 * @param [in] ptrBuffer pointer to the user input buffer.
 */
void setUserInputBuffer(uint8_t *ptrBuffer){
	userInputBuffer = ptrBuffer;
}


/**
 * The user is prompted to input some data to send through the BT device
 */
static uint8_t getDataFromUser(void){
	uint8_t tmp[64];
	memset(tmp,0,sizeof(tmp));
	uint8_t data[64];
	memset(data,0,sizeof(data));
	if(userInputBuffer[0] != 0 && userInputBuffer[1] != 0 &&
			userInputBuffer[2] != 0){
		sscanf((const char *)userInputBuffer,"%s %s", data, tmp);
		if(strcmp((const char *)tmp,"end") == 0){
			sprintf((char *)tmp,"The following data will be send : ");
			HAL_UART_Transmit(&userHuart, tmp, sizeof(tmp),10);
			HAL_UART_Transmit(&userHuart,data,sizeof(data),10);
			memset(tmp,0,sizeof(tmp));
			sprintf((char *)tmp, "\r\n");
			HAL_UART_Transmit(&userHuart, tmp, sizeof(tmp),10);
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
 * @param in dataLength the number of
 */
static void sendDataToDevice(uint8_t * dataBuffer, uint16_t dataLength){
	uint8_t *commandBuffer = malloc(sizeof(uint8_t)*(dataLength+2));
	uint16_t i = 0;
	commandBuffer[0] = 0x03;
	commandBuffer[1] = dataLength+2;
	for(i=0; i<dataLength; i++){
		commandBuffer[i+2] = dataBuffer[i];
	}
	HAL_UART_Transmit(&userHuart,commandBuffer,sizeof(commandBuffer),10);
}


/**
 * User is asked to input a new value for the periodic timer period.
 */
static uint8_t setTimerPeriod(void){
	uint8_t tmp[64];
	memset(tmp,0,sizeof(tmp));
	uint32_t tmpTimerPeriod = 0;
	if(userInputBuffer[0] != 0 && userInputBuffer[1] != 0 &&
			userInputBuffer[2] != 0){
		sscanf((const char *)userInputBuffer,"%lu %s", &tmpTimerPeriod, tmp);
		if(strcmp((const char *)tmp,"end") == 0){
			timerPeriod = tmpTimerPeriod;
			memset(tmp,0,sizeof(tmp));
			sprintf((char *)tmp,"New timerPeriod value : %lu\r\n",timerPeriod);
			HAL_UART_Transmit(&userHuart, tmp, sizeof(tmp),10);
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
	HAL_UART_Transmit(&userHuart, (uint8_t *)"\r\nAutoMode Started !\r\n", sizeof("AutoMode Started !"),10);
	autoModeStatus = AUTOMODE_ON;
}


/**
 * Stop to send the temperature periodically
 */
static void stopAutoMode(void){
	HAL_UART_Transmit(&userHuart,(uint8_t *)"\r\nAutoMode Stopped !\r\n", sizeof("AutoMode Stopped !"),10);
	autoModeStatus = AUTOMODE_OFF;
}


/******************************
 * Commands Handler functions
 ******************************/
static void menuHandler(void){
	displayMenu();
}
static void rsHandler(void){
	HAL_UART_Transmit(&userHuart, (uint8_t *)"Input buffer reset\r\n",sizeof("Input buffer reset\r\n"),10);
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
	// I dont have a decent solution right now
}


static void rfSignalCheckHandler(void){
	uint8_t command[3];
	uint8_t buffer[] = "\r\nrfsignalcheck command was sent\r\n";
	command[0] = 0x01; command[1] = 0x01; command[2] = 0x01;
	HAL_UART_Transmit(&deviceHuart, command,sizeof(command),10);
	HAL_UART_Transmit(&userHuart, buffer, sizeof(buffer),10);
}

static void networkJoinHandler(void){
	uint8_t command[3];
	uint8_t buffer[] = "\r\nnetworkjoin command was sent\r\n";
	command[0] = 0x02; command[1] = 0x01; command[2] = 0x01;
	HAL_UART_Transmit(&deviceHuart, command,sizeof(command),10);
	HAL_UART_Transmit(&userHuart, buffer,sizeof(buffer),10);
}


static void sendSampleDataHandler(void){
	uint8_t command[7];
	command[0] = 0x01; command[1] = 0x05; command[2] = 0x01;
	command[3] = 0x02; command[4] = 0x03; command[5] = 0x04;
	command[6] = 0x05;
	HAL_UART_Transmit(&deviceHuart, command,sizeof(command),10);
	HAL_UART_Transmit(&userHuart, (uint8_t *)"datatransfer\r\n",
			sizeof("datatransfer\r\n"),10);
	uint8_t txBuffer[64];
	memset(txBuffer,0,sizeof(txBuffer));
	sprintf((char *)txBuffer,"This data was sent : %d %d %d %d %d\r\n",command[2],command[3],
			command[4],command[5],command[6]);
	HAL_UART_Transmit(&userHuart,txBuffer, sizeof(txBuffer), 10);;
}

static void getTimerPeriodHandler(void){
	uint32_t timerPeriodValue = BTDevice_getTimerPeriod();
	uint8_t txBuffer[64];
	memset(txBuffer,0,sizeof(txBuffer));
	sprintf((char *)txBuffer,"Current timer period value : %lu\r\n",timerPeriodValue);
	HAL_UART_Transmit(&userHuart,txBuffer,sizeof(txBuffer),10);
}

static void setTimerPeriodHandler(void){
	setUserUartInTimerSettingsMode();
}

/*****************************
 * BTDevice public functions
 *****************************/
uint32_t BTDevice_getTimerPeriod(){
	return timerPeriod;
}

uint32_t BTDevice_getPeriodCounter(){
	return periodCounter;
}


uint8_t BTDevice_getAutoModeStatus(void){
	return autoModeStatus;
}


void BTDevice_incrementPeriodCounter(){
	periodCounter++;
}

uint8_t BTDevice_readInputBuffer(){
	uint8_t commandIterator;
	uint8_t commandRecognized = 0;
	uint16_t len = strlen((const char *)userInputBuffer);
	HAL_UART_Transmit(&userHuart, &userInputBuffer[len-1],sizeof(uint8_t),10);
	switch(getUserUartMode()){
	case  COMMAND_MODE:
		if(checkForInputBufferReset() == 1){
			rsHandler();
			return RESET_BUFFER;
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
			return RESET_BUFFER;
		}
		break;
	case DATA_INPUT:
		if(getDataFromUser() == RESET_BUFFER){
			return RESET_BUFFER;
		}
		break;
	case TIMER_SETTINGS:
		if(setTimerPeriod() == RESET_BUFFER){
			return RESET_BUFFER;
		}
		break;
	}
	return NO_RESET;
}


void BTDevice_init(BTDevice_InitTypeDef *BTDevice_InitStruct){
	setUserHuart(BTDevice_InitStruct->userHuart);
	setDeviceHuart(BTDevice_InitStruct->deviceHuart);
	setUserInputBuffer(BTDevice_InitStruct->userInputBuffer);
}

void BTDevice_sendData(uint8_t *dataBuffer, uint16_t dataLength){

	uint8_t txBuffer[64];
	memset(txBuffer,0, sizeof(txBuffer));
	//First we extract the data value from the data buffer
	memset(txBuffer, 0, sizeof(txBuffer));
	if(BTDevice_getAutoModeStatus() == AUTOMODE_ON){
		sprintf((char *)txBuffer,"AutoMode is ON\r\n");
		HAL_UART_Transmit(&userHuart, txBuffer,sizeof(txBuffer),10);
	}else{
		sprintf((char *)txBuffer,"AutoMode is OFF\r\n");
		HAL_UART_Transmit(&userHuart, txBuffer,sizeof(txBuffer),10);
	}
	sendDataToDevice(dataBuffer,dataLength);
}




void BTDevice_resetPeriodCounter(){
	periodCounter = 0;
}

void BTDevice_displayMenu(void){
	displayMenu();
}



