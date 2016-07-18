/**
* @brief This function handles USARTX global interrupt.
*/

//Make you peripheral handle visible from this file
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef husart6;

extern TIM_HandleTypeDef htim1;


//Call the HAL IRQHandler. It will wait the required amount of time and then call the callback.
void TIM1_IRQHandler(void){
	HAL_TIM_IRQHandler(&htim1);
}

void USART2_IRQHandler(void){
	HAL_UART_IRQHandler(&huart2);
}

//Please replace USART6 by the right usart/uart number
//Call the HAL_IRQHandler. It will receive the specified amout of data and the call the callback
void USART6_IRQHandler(void)
{
	HAL_UART_IRQHandler(&husart6);
}

