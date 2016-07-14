/*
 * stm32fxxx_hal_uart_it.h
 *
 *  Created on: Jun 25, 2016
 *      Author: constantin.chabirand@gmail.com
 */

#ifndef STM32FXXX_HAL_UART_IT_H_
#define STM32FXXX_HAL_UART_IT_H_


/**
 * @brief  Enable infinite IT
 * @param  huart: pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @param  pData: Pointer to data buffer where data will be received
 * @param  Size: 1 or more. Set to 1 later on anyway.
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_IT_Enable(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);


/**
 * @brief Disable infinite IT
 * @param  huart: pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval HAL Status
 */
HAL_StatusTypeDef HAL_UART_IT_Disable(UART_HandleTypeDef *huart);


/**
 * @brief IRQ Handler for infinite IT. HAL_UART_RxCpltCallback(huart) is called when finished.
 * @param  huart: pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @retval HAL Status
 */
void HAL_UART_IT_IRQHandler(UART_HandleTypeDef *huart);



#endif /* APPLICATION_USER_STM32FXXX_HAL_UART_IT_H_ */

