/*
 *  * @brief Driver to use and control a BluTech LoRa device
 *   * @file stm32fxxx_hal_BTDevice.h
 *    * @author constantin.chabirand@gmail.com
 *     *
 *      * Please refer to the tutorial and example available on github :
 *       * https://github.com/CynaCons/stm32_BluTechDevice
 *        */

#ifndef STM32FXXX_HAL_BTDEVICE_H_
#define STM32FXXX_HAL_BTDEVICE_H_



/**
 * @typedef BTDevice_InitTypeDef
 * @brief Structure to be filled with references in order to initialize the library
 * 	Initialization is done by calling BTDevice_Init()
 */
typedef struct {
	UART_HandleTypeDef *userHuart; /* Pointer to the userUart handler structure */
	UART_HandleTypeDef *deviceHuart; /* pointer to the deviceUart handler structure */
	uint8_t *userInputBuffer; /* reference to the buffer filled by the user input */
	void (*resetInputBufferHandler)(void); /* function pointer to function that reset the userInputBuffer and make it ready to be filled again. */
	void (*deviceCommandReceivedHandler)(uint8_t *dataBuffer, uint16_t dataLength); /* func pointer to routine that should handle the data/commands
	received by the device from the gateway */
	void (*dataSentCallback)(void); /*!< Function pointer to the function to call when data has been sent */
	uint8_t * (*getSensorDataFunction)(void); /*!< Function pointer to the function returning the sensor data in JSON format */
	uint16_t sensorDataMaxLength; /*!< Maximum length of the sensor data */
} BTDevice_InitTypeDef;


/**
 * @typedef BTDevice_Status
 * @brief Enumeration for the library return values
 */
typedef enum{
	BTDevice_OK = 0,
	BTDevice_ERROR
}BTDevice_Status;


/**
 * @typedef BTDevice_AutoInitTypeDef
 * @brief Structure to be filled and given to BTDevice_autoInitWithDefaultValues
 */
typedef struct{
	uint32_t timerPeriodValue; //Any positive value but 0
	uint8_t autoModeStatus; // Fill with a AutoModeStatus enum value
}BTDevice_AutoInitTypeDef;

//TODO Write that fking BTDevice_ErrorHandler
/**
 * @typedef BTDevice_Error
 * @brief Error code to be given to the BTDevice_ErrorHandler
 */
typedef enum{
	BT_HAL_UART_ERROR = 0,
	BT_DEVICE_ERROR,
	BT_SENSOR_ERROR
}BTDevice_Error;

/**
 * @typedef AutoModeStatus
 * @brief State of the periodic data transfer mode
 */
typedef enum {
	AUTOMODE_OFF, /*!< OFF = no data will be sent periodically */
	AUTOMODE_ON /*!<ON = data will be sent every period. Period is set using menu */
}AutoModeStatus;


/**
 * This function should be called from the HAL_UART_RxCompleteCallback function
 * It will signal that a character was received from the device which triggers processing of the received data in the mainLoop
 * @param [in] deviceUartRxBuffer Single byte buffer containing last received value from the BTDevice
  * 	After reading the head of a message, the rest of the message reception will be handled inside the library
 * 		The result will be prompted to user through user uart
 */
void BTDevice_deviceUartCallback(uint8_t *deviceUartRxBuffer);


/**
 *   Display the UI menu via the user uart.
 *   @pre user uart must have been set before
 */
void BTDevice_displayMenu(void);


/*
 * @brief Initialize the BTDevice drivers using an init structure.
 * @param [in] *BTDevice_InitStruct the structure to fill in order to complete the initialization process
 */
void BTDevice_init(BTDevice_InitTypeDef *BTDevice_InitStruct);
//TODO Add error handling

/**
 * his function should be called from UARTx_RxCpltCallback
 * It will signal that a character was received on the userUart which signal the mainLoop to analyse the received data
 * @param [in] *userUartRxBuffer single byte buffer to receive characters from the input one by one
 * @pre user uart must have been set before
 * @pre the user rxBuffer must have been set before
 */
void BTDevice_userUartCallback(uint8_t *userUartRxBuffer);


/**
 * This function must be called from the 'timer period up callback'.
 * It will signal that one second has ellasped and will trigger further processing
 * by the mainLoop
 * @pre the interupt which triggered this 'tim up it' must occur every second
 */
void BTDevice_timerCallback(void);


/**
 * This function is the mainLoop of the library. It should be called indefinitely from user space
 *
 * It will check for the peripherals flags (UART IT or TIM IT) evoqued above and will call the appropriate functions depending on the situation.
 *
 * There are cases :
 * 	 	=> A character was received on the userUart
 * 	 	=> A character was received on the deviceUart
 * 	 	=> A timer IT (every second) has occurred
 */
void BTDevice_mainLoop();

/**
 * This function will initialize the library according to the values specified in the init structure
 *
 * It will loop untill the gateway network is joined.
 *
 * When the intialisation process fail, it will call the initFailed function.
 * If the process is successful, it will call the initSucces function.
 */
void BTDevice_initLoop();


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
void BTDevice_sendData(uint8_t *dataBuffer, uint16_t dataMaxLength);
//TODO Add error hanlding possibilities



/*
 * Initialize settings with default values provided in defaultValue.
 * This function will set the timer period value, set automode status and perform a network join.
 *
 * @param [in] defaultValues A structure filled with default values to be used during automatic initialization
 * @return BTDevice_OK if init worked, BTDevice_ERROR otherwise
 */
BTDevice_Status BTDevice_autoInitWithDefaultValues(BTDevice_AutoInitTypeDef defaultValues);

#endif /* STM32FXXX_HAL_BTDEVICE_H_ */


