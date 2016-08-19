/*
 * @brief Driver to use and control a BluTech LoRa device
 * @file stm32fxxx_hal_BTDevice.h
 * @author constantin.chabirand@gmail.com
 *
 * Please refer to the tutorial and example available on github :
 * https://github.com/CynaCons/stm32_BluTechDevice
 */

#ifndef STM32FXXX_HAL_BTDEVICE_H_
#define STM32FXXX_HAL_BTDEVICE_H_



/**
 * Init structure that contains references to the peripherals (UART), to the user input buffer.
 * It also contains a reference to the user-defined function that must be called to reset input buffer
 */
typedef struct {
	UART_HandleTypeDef *userHuart;
	UART_HandleTypeDef *deviceHuart;
	uint8_t *userInputBuffer;
	void (*resetInputBufferHandler)(void);
	void (*deviceCommandReceivedHandler)(uint8_t *dataBuffer, uint16_t dataLength);
} BTDevice_InitTypeDef;


/**
 * This function should be called from the HAL_UART_RxCompleteCallback function
 * It will handle data and command answers from BTDevice to MCU
 * @param [in] deviceUartRxBuffer Single byte buffer containing last received value from the BTDevice
 * 				After reading this value, the rest of the BTDevice answer's will be received inside the drivers
 * 				The result will then be prompted to uset through user uart
 */
void BTDevice_deviceUartCallback(uint8_t *deviceUartRxBuffer);


/**
 * Display the UI menu via the user uart.
 * @pre user uart must have been set before
 */
void BTDevice_displayMenu(void);


/**
 * Initialize the BTDevice drivers using an init structure.
 */
void BTDevice_init(BTDevice_InitTypeDef *BTDevice_InitStruct);


/**
 * This function should be called by the user uart rxCompleteCallback to read the input buffer and look for known commands
 * @pre user uart must have been set before
 * @pre the user rxBuffer must have been set before
 */
void BTDevice_readInputBuffer(void);


/**
 * This function must be called from the 'timer period up callback'.
 * It will increment the number of seconds waited since last data transfer.
 * This number of seconds is then compared to the timer period value (set with userUart).
 * If enough time has been waited, it returns 1 otherwise returns 0
 */
uint8_t BTDevice_timerCallback(void);


/**
 * Send the specified data to the device through the device uart
 * Please make sure to convert the data to a uint8_t buffer using sprintf
 * @param [in] txBuffer pointer to the data buffer to send
 * @param [in] txLength the length of the data buffer to send
 */
void BTDevice_sendData(uint8_t *dataBuffer, uint16_t dataMaxLength);



#endif /* STM32FXXX_HAL_BTDEVICE_H_ */

