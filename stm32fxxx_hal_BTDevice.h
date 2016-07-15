/*
 * stm32fxxx_hal_BTDevice.h
 *
 *  Created on: Jul 6, 2016
 *      Author: cynako
 */

#ifndef STM32FXXX_HAL_BTDEVICE_H_
#define STM32FXXX_HAL_BTDEVICE_H_


typedef enum {
	AUTOMODE_OFF,
	AUTOMODE_ON
}AutoModeStatus;

//REFACTOR, We repeat ourselves
typedef enum{
	COMMAND_MODE,
	TIMER_SETTINGS,
	USER_INPUT_FINISHED,
	DATA_INPUT
}UserUartMode;

typedef enum{
	NO_RESET,
	RESET_BUFFER
}InputBufferStatus;

typedef struct {
	UART_HandleTypeDef *userHuart;
	UART_HandleTypeDef *deviceHuart;
	uint8_t *userInputBuffer;
	void (*resetInputBufferHandler)(void);
} BTDevice_InitTypeDef;


/****************************
 * User interface functions
 ****************************/


/**
 * Display the UI menu through the user uart.
 * @pre user uart must have been set before
 */
void BTDevice_displayMenu(void);

/**
 * This function should be called by the user uart rxCompleteCallback to read the input buffer and look for known commands
 * @pre user uart must have been set before
 * @pre the user rxBuffer must have been set before
 */
void BTDevice_readInputBuffer(void);



uint8_t BTDevice_timerCallback(void);

/**********************
 * AutoMode functions
 **********************/


/**
 * Enable the AutoMode (periodic transfer of data)
 */
//void BTDevice_startAutoMode(void);


/**
 * Disable the AutoMode (periodic transfer of data)
 */
//void BTDevice_stopAutoMode(void);


/**
 * Return whether or not the auto mode is enable
 * @return 1 = AutoMode enabled ; 0 = AutoMode disabled
 */
uint8_t BTDevice_getAutoModeStatus(void);


/***************************
 * Timer control functions
 ***************************/


/**
 * Set the Timer period for the AutoMode.
 * The user will be asked to type the value followed by 'end'
 * @pre the timer must have been set before
 */
//void BTDevice_setTimerPeriod();


/**
 * Returns the current timer period
 */
//uint32_t BTDevice_getTimerPeriod();


/**
 * Returns the current number of period that occured since last data sending (one period = one PeriodElapsedCallback = one TIMx UP IT)
 */
//uint32_t BTDevice_getPeriodCounter();


/**
 * Add +1 to the period count value
 */
//void BTDevice_incrementPeriodCounter();


/**
 * Initialize the BTDevice drivers
 */
void BTDevice_init(BTDevice_InitTypeDef *BTDevice_InitStruct);


/**
 * Set period counter to 0
 */
//void BTDevice_resetPeriodCounter();


/*****************************
 * Sensor specific functions
 *****************************/

/**
 * Send the specified data to the device through the device uart
 * Please make sure to convert the data to a uint8_t buffer using sprint
 * @param [in] txBuffer pointer to the data buffer to send
 * @param [in] txLength the length of the data buffer to send
 */
void BTDevice_sendData(uint8_t *dataBuffer, uint16_t dataMaxLength);



/******************
 * UART Fonctions
 ******************/


/* Unused function since using a Timer is more accurate than using Systick IRQs */
//void BTDevice_getSystickValue(void);//TODO Incoherent that get returns void
//void BTDevice_setSystickValue(void);

#endif /* STM32FXXX_HAL_BTDEVICE_H_ */

