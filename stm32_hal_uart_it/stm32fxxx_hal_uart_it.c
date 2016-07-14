/**
 ******************************************************************************
 * @file    stm32fxxx_hal_uart_it.c
 * @author Constantin Chabirand
 * @version 0.1.1
 * @date June 29th, 2016
 * @brief Allow to use the interruption process without limits or constraints. This is based on ST's HAL library with a few tweaks
 ******************************************************************************
 * @attention
 *
 * <h2><center>&copy; COPYRIGHT(c) 2016 STMicroelectronics</center></h2>
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

#if defined(STM32F103RCTx) || defined(STM32F103xx) || defined(STM32F103)|| defined(STM32F103xE)
#include "stm32f1xx_hal.h"
#endif

#if defined(STM32F4DISCOVERY)|| defined(STM32F407VGTx)|| defined(STM32F407xx)
#include "stm32f4xx_hal.h"
#endif


static HAL_StatusTypeDef UART_IT_Receive(UART_HandleTypeDef *huart);


static uint8_t *huartITRxBuffer = 0;

HAL_StatusTypeDef HAL_UART_IT_Enable(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
{
	/*
	 * Cynako : This is to store the data buffer's address so that we can use it for each IT
	 */
	huartITRxBuffer = pData;
	/* Check that a Rx process is not already ongoing */
	if(huart->RxState == HAL_UART_STATE_READY)
	{
		if((pData == NULL ) || (Size == 0U))
		{
			return HAL_ERROR;
		}

		/* Process Locked */
		__HAL_LOCK(huart);

		huart->pRxBuffPtr = pData;
		huart->RxXferSize = Size;
		huart->RxXferCount = Size;

		huart->ErrorCode = HAL_UART_ERROR_NONE;
		huart->RxState = HAL_UART_STATE_BUSY_RX;

		/* Process Unlocked */
		__HAL_UNLOCK(huart);

		/* Enable the UART Parity Error Interrupt */
		SET_BIT(huart->Instance->CR1, USART_CR1_PEIE);

		/* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
		SET_BIT(huart->Instance->CR3, USART_CR3_EIE);

		/* Enable the UART Data Register not empty Interrupt */
		SET_BIT(huart->Instance->CR1, USART_CR1_RXNEIE);

		return HAL_OK;
	}
	else
	{
		return HAL_BUSY;
	}
}

/**
 * @brief  Receives an amount of data in non blocking mode
 * @param  huart: pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @param  pData: Pointer to data buffer
 * @param  Size: Amount of data to be received
 * @retval HAL status
 */
HAL_StatusTypeDef HAL_UART_IT_Disable(UART_HandleTypeDef *huart)
{
	/* Enable the UART Parity Error Interrupt */
	CLEAR_BIT(huart->Instance->CR1, USART_CR1_PEIE);

	/* Enable the UART Error Interrupt: (Frame error, noise error, overrun error) */
	CLEAR_BIT(huart->Instance->CR3, USART_CR3_EIE);

	/* Enable the UART Data Register not empty Interrupt */
	CLEAR_BIT(huart->Instance->CR1, USART_CR1_RXNEIE);

	return HAL_OK;
}



void HAL_UART_IT_IRQHandler(UART_HandleTypeDef *huart)
{
	/*
	 * Cynako : No error related code was added.
	 * 			No code related to transmit mode was added.
	 *
	 */
	UART_IT_Receive(huart);
	return;
}

/**
 * @brief  Receives an amount of data in non blocking mode
 * @param  huart: pointer to a UART_HandleTypeDef structure that contains
 *                the configuration information for the specified UART module.
 * @pre    HAL_UART_IT_Enable must have been called before
 * @retval HAL status
 */
static HAL_StatusTypeDef UART_IT_Receive(UART_HandleTypeDef *huart)
{
	uint16_t* tmp;

	/*
	 * Cynako : Value received in huart->DR will be transfered to previously set huartITRxBuffer
	 * .
	 */
	huart->pRxBuffPtr = huartITRxBuffer;

	/* Check that a Rx process is ongoing */

	/*
	 *	Cynako : The following if--else was not modified
	 */
	if(huart->RxState == HAL_UART_STATE_BUSY_RX)
	{
		if(huart->Init.WordLength == UART_WORDLENGTH_9B)
		{
			tmp = (uint16_t*) huart->pRxBuffPtr;
			if(huart->Init.Parity == UART_PARITY_NONE)
			{
				*tmp = (uint16_t)(huart->Instance->DR & (uint16_t)0x01FFU);
				huart->pRxBuffPtr += 2U;
			}
			else
			{
				*tmp = (uint16_t)(huart->Instance->DR & (uint16_t)0x00FFU);
				huart->pRxBuffPtr += 1U;
			}
		}
		else
		{
			if(huart->Init.Parity == UART_PARITY_NONE)
			{
				*huart->pRxBuffPtr++ = (uint8_t)(huart->Instance->DR & (uint8_t)0x00FFU);
			}
			else
			{
				*huart->pRxBuffPtr++ = (uint8_t)(huart->Instance->DR & (uint8_t)0x007FU);
			}
		}
		/*
		 *  Cynako :
		 *	The following part was supposed to disable IT when the required number
		 *	of characters were transmitted.
		 *	It has been replaced by the following code to enable infinite IT
		 */
		huart->RxXferCount = 1;// 1 character still to transfer
		huart->RxXferSize = 1;// 1 character still to transfer
		huart->ErrorCode = HAL_UART_ERROR_NONE; //No errors
		huart->RxState = HAL_UART_STATE_BUSY_RX; //UART will be ready to receive using IT
		__HAL_UNLOCK(huart); //UART unlocked so we dont miss the next character
		HAL_UART_RxCpltCallback(huart); //Call the callback to handle the new received value
		return HAL_OK;
	}
	else
	{
		return HAL_BUSY;
	}
}




