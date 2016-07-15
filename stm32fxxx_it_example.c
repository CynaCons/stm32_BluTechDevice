/**
* @brief This function handles USARTX global interrupt.
*/

//Please replace USART6 by the right usart/uart number
void USART6_IRQHandler(void)
{
	HAL_UART_IRQHandler(&husart6);
}

