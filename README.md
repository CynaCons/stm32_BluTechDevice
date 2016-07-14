# stm32_BluTechDevice
Drivers to command the BluTech devices using an STM32 device


                                                                          XXXXX
                                          +display commands menu          XXXXX
                                          +display values which are sent  XXXXX
+-------+   sensor data+--------------+                                     X       You can plug to the device to input
|       |              |              | ---------------------------->       X       some settings or check if it's working
| sensor+-------------->              |                                 XXXXXXXXXX
|       |              |              |                                     X       Once the settings are finished,the
+-------+              |   STM32Fxxx  |        user uart                    X       devicewill work on it's own.
                       |              |                                     X
                       |              |                                    XXX
                       |              | ----------------------------+    XXX XXX
                       +--------------+                                XXX     XXX
                                         +perform commands (like
                                      ^  network join)
                       +              |  +input settings (timer period) The user !
                       |              |  +send sample data & other
                       |              |
                       |              |
                       |    de^ice    |
                       |    uart      | +Response following a command
       UART Commands   |              |
       (like networkjoi|              | +Commands from REST API
       or datatransfer)|              |
                       |              +
                       v

                      +---------------+
                      |               |
                      |               |
                      |     BluTech   |
                      |     device    |
                      |               |
                      |               |
                      +---------------+


How to use ? 

This driver uses keyboard input without knowing the input length in advance. Input is handled through material interrupts. Since HAL library doesnt allow to use interrupts without knowing the length in advance, I use my own modified version of some hal_uart functions.

I highly recommend to use an STM32CubeMX generated project with the HAL library.

You will find details for each function the example files (main.c, stm32fxxx_it.c)


# In your main.c : 


  
Then include the required files : 

      #include "stm32fxxx_hal_BTDevice.h" 
      #include "stm32fxxx_hal_uart_it.h"


Declare what you need to handle the input from keyboard

      uint8_t *rxITBuffer[256];
      uint16_t rxITIndex = 0

Initialize your board including : 
  - 2 uart : one for sending commands to the BluTech device, the other to interact with the used using UART interface.
  - 1 timer : data is send periodically using a timer. 
  - What you need to acquire sensor data (ADC)

Start the custom UART IT and the timer

      	HAL_UART_IT_Enable(&huart6, &husart6RxBuffer,1);
      	HAL_TIM_Base_Start_IT(&htim1);


Then init BTDevice and display the command menu on the user uart
    
        initBTDevice();
    	BTDevice_displayMenu();


Write your timer callback that will be called after each period to send the data : 

      void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
        //Look at the example ! 
      }


Write the uart interupt callback : 

      void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
        //Look at the example ! 
      }



# In your stm32fxxx_it.c : 

We cannot call the default HAL interrupt handler since it's not capable to receive an undefined amout of data. Instead we use my own modified version. It's like the default one except it doesnt disable uart interupt when receiving is complete.


Write the uart IRQHandle for your user uart : 

      void USART6_IRQHandler(void)
      {
        //Make sure you dont call HAL_UART_IRQHandler(&huart6) otherwise IT will be disabled
      
          //This will call custom IRQHandler that will let IT enabled. USART6 is my user uart.
      	HAL_UART_IT_IRQHandler(&huart6);
      }

