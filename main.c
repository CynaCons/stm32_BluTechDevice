/**
 *  Include your stuff here
 */

//Include the BTDevice header file
#include "stm32fxxx_hal_BTDevice.h" 

//Include the hal_uart_it header file for "infinite" IT. Just like circular DMA
#include "stm32fxxx_hal_uart_it.h"

//Use buffer to store data received from user uart
uint8_t *rxITBuffer[256];

//This is the input buffer index. When a command is recognized, the buffer is cleaned and index is reset
uint16_t rxITIndex = 0


int main(void)
{
	/**
	 *   Init your stuff
     *   You have to init two UART, one TIMER and maybe an ADC.
     *   Example below : 
     */ 
    MX_GPIO_Init();
	MX_DMA_Init();
	MX_ADC1_Init();
	MX_USART1_Init();
	MX_USART2_UART_Init();
	MX_USART6_UART_Init();
	MX_TIM1_Init();

    //Clean input buffer
	memset(rxITBuffer,0,sizeof(rxITBuffer));

    //Using custom function, enable infinite IT (just like circular DMA)
	HAL_UART_IT_Enable(&huart6, &husart6RxBuffer,1);

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
	BTDevice_InitStruct.userHuart = &huart6;
	BTDevice_InitStruct.deviceHuart = &huart2;
	BTDevice_InitStruct.userInputBuffer = rxITBuffer;

	BTDevice_init(&BTDevice_InitStruct);
}


/**
 * TIMER Callback function. Periodically, data will be sent.
 * Period value can be set through uart interface (userHuart)
 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
    //Make sur it's the right timer 
	if(htim->Instance == TIM1){ 
		if(BTDevice_getAutoModeStatus() == AUTOMODE_ON){
			BTDevice_incrementPeriodCounter();
			if(BTDevice_getPeriodCounter() >= BTDevice_getTimerPeriod()){
				BTDevice_resetPeriodCounter();
			   
                 //Get your data the way you want
                uint8_t dataBuffer[dataLength] = getMySensorData();

                //The send it
				BTDevice_sendData(dataBuffer,dataLength);
			}
		}
	}
}


void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	//UART 2 is my device uart. Here you process what's coming from the BTDevice 
    if(huart->Instance == USART2){
		
	}
    //USART6 is my user uart. Here you process what the user typed on his keyboard.
	if(huart->Instance == USART6){

		storeNewValueIntoInputBuffer();//Store new value in rxITBuffer and increase rxITIndex

        //Read input buffer. The function returns RESET_BUFFER when buffer needs to be cleaned
		switch(BTDevice_readInputBuffer()){
		case RESET_BUFFER:
			resetInputBuffer();
			break;
		}
	}
}


/**
 * Example for handling incoming characters from uart input
 */
void storeNewValueIntoInputBuffer() {
	rxITBuffer[rxITIndex] = husart6RxBuffer;
	rxITIndex++;
}


/**
 * Example to reset the inputBuffer.
 */
void resetInputBuffer() {
	memset(rxITBuffer, 0, sizeof(rxITBuffer));
	rxITIndex = 0;
}




