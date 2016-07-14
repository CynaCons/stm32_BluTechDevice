/**
* @brief This function handles USART6 global interrupt.
*/
void USART6_IRQHandler(void)
{

  //Make sure you dont call HAL_UART_IRQHandler(&huart6) otherwise IT will be disabled


    //This will call custom IRQHandler that will let IT enabled. USART6 is my user uart.
	HAL_UART_IT_IRQHandler(&huart6);
}

