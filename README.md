# stm32_BluTechDevice

#What is this repository ?

This repository contains : 

    - the core files of the library : stm32fxx_hal_BTDevice.c and stm32fxxx_hal_BTDevice.h
    - example projects in the 'projects' folder
    - this README.md file

The core .c and .h files must be included to a stm32 project to use and control a BluTech LoRa device via UART.
Examples are provided to build a functionnal program quickly and easily using public functions from the library.

Currently, three boards are supported : stm32f4-disco ; stm32f103 ; stm32l152.

This library is built to be generic and easily reused across all stm32 projects, only the data sensing part has to change. It is build on ST's HAL library and uses the STM32CubeMX project architecture.

Check the provided example project for copyable code. Only the lightSensor project is fully commented, check this one first.

   

# How to get started : 

	-Clone this repository. 
	-Choose a project and open it (IAR project file can be found in EWARM folder)
	(optionnal) Write the appropriate code to retrieve data from your sensor.
	-Make sur VLA is enabled in the project settings (compiler tab), otherwise you won't compile. Then configure the target in the project options.
	-Compile,flash your board and wire the UARTs :(pin numbers can be found on the hal_msp.c for each project) one UART for communication with the BTDevice, one UART to your computer to write the commands
	-Restart the board and you should now see the commands menu.


## How does it work ? (visual description at the end of this file) : 

This librabry will use two UART :

The first one, called "deviceUart" will send commands to the BluTech device and receive the command's answer or receive data sent from the REST API

The other, called "userUart", will print a list of commands that can be used to control the BluTech device. It also display the command's result and a lot of other useful things (such as sensor data).	

The second UART is optionnal, only plug it to setup the device and then you can unplug it and move to something else.

Follow the guide below or read the source file in the example projects to write the appropriate code to have a functionnal device. 

## Functionnalities 

Once it is functionnal this library allows you to use the following commands : 

	= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 
				                APPLICATION MENU		                       	
		                                                                        
	
	+++   GENERAL CONTROL
	    --> menu                : display this menu
	    --> device status       : display a status report for this device
	    --> rs                  : reset the input buffer (in case of typing mistake)
	    --> get last value      : display the last sensor value
	    --> set automode on     : start sending data periodically
	    --> set automode off    : stop sending data periodically
	    --> get automode status : display the automode status (ON or OFF)
	
	+++   BLUTECH DEVICE CONTROL
	    --> rf signal check     : perform a signal check
	    --> network join        : join the gateway network
	    --> get network status  : display the network status (CONNECTED or not)
	    --> flush sensor data   : instantly send sensor data and reset the timer period
	    --> send data           : input some ascii data that will be sent to the gateway
	    --> send sample data    : send a sample of data for testing
	
	+++   TIMER CONTROL
	    --> set timer period    : set the timer period
	    --> get timer period    : get the current timer period
	
	WARNING : You have to join a network in order to send data through the LoRa module
	= = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = 


You can for example configure you device the send the sensor's data periodically, configure the period, perform a signal test or send any data that you input with yout keyboard. 

##Example projects

Example projects can be found in the "Projects" folder.

The project architecture is the following :

		.
		├── BluTechDevice_stm32l152_lightSensor.ioc //STM32CubeMX project file
		├── Drivers //HAL library and board related files
		├── EWARM //IAR project files
		├── Inc //Header file
		│   ├── buzzer.h 
		│   ├── mxconstants.h
		│   ├── stm32fxxx_hal_BTDevice.h //Library code header file
		│   ├── stm32l152c_discovery.h
		│   ├── stm32l1xx_hal_conf.h
		│   └── stm32l1xx_it.h 
		├── Src
		│   ├── buzzer.c //C Source file to play sound with the buzzer
		│   ├── main.c //C Source file to init the peripherals, call the library public functions and handle peripherals callbacks
			       // main.c will also setup HAL, config the system clock and start the peripheral init procedure that call   // hal_msp.c functions
		│   ├── stm32fxxx_hal_BTDevice.c //Library core C Source file
		│   ├── stm32l152c_discovery.c
		│   ├── stm32l1xx_hal_msp.c //C Source file to initializa low level peripherals (USART / TIMER pins, IT, NVIC ...)
		│   └── stm32l1xx_it.c // C Source file where peripherals IRQHandler will can the appropriate HAL_IRQHandler (and then 				// the appropriate HAL_XX_Callback in main.c)
		└── SW4STM32 //Eclipse(SW4STM32) project files



#How to make a device to send periodically the sensor data ?

## AutoInit

"AutoInit" refers to including the initialization process (joining the gateway's network, setting the timer period for data transfer, enabling the periodic data transfer) directly in the main() from main.c. 

This way, when the device is powered, it will work on it's own, no need to configure or wire anything (userUart)

To implemet AutoInit, refer to the example projects (read carefully before the mainLoop for the autoInit procedure)

##Manually

First, flash your MCU and include the library (see example projects for more details). 
Then wire your MCU to the sensor and to the BTDevice (thats deviceUart)
Wire one UART/USB cable between your computer and the BTDevice (userUart)
Then restart the MCU and you should now see the application menu !

*Once you can access the application menu :*

=> set the period in seconds between two data transfers : 

	
		set timer period
		[...]
		XX end (where XX is a value in seconds, like 180)
	
=> join the LoRa network :
 
	
		network join
		[...]
		Network join confirmation was received
	
=> enable periodic data transfer according to the period you have set :
	
	
		set automode on
		[...]
		AutoMode Started !
		[...]
		AutoMode ON
		The following data was just sent : YYY (where YYY is the sensor data)
	
    Thats it ! Now every XX seconds, your sensor data will be sent

=> you can still input commands while automode is ongoing. You can stop automode using :
	
	
		set automode off
		[...]
	
=> if you want to reset the input (for example when you have done a typing mistake) :

        set automodeo nn <<<<This is so wrong !! 
        rs
        Input buffer reset. Start typing a new command
        set automode on <<<<<Much better ! 
       
    Whenever you type something wrong in the UART terminal, just do a quick rs!
		
	


# How to create a new, standalone project

I highly recommend to use the STM32CubeMX project generator. This lirabry uses the HAL library.
STM32CubeMX will generate the project architecture and initialize all the required peripherals.

STM32CubeMX can be found on ST's website and can generate projects for most IDEs.

To understand how to use, read the example projects.


## Public functions

The library's functionnalities are available through public functions. 

All public functions are formatted this way : BTDevice_functionName.

###Init functions
The initialisation procedures (init, and autoInit) are associated with an InitStructure.

To perform the init process, simply declare and fill the appropriate structure (cf example projects) and call the BTDevice_XXXinit function

###Callback functions 

Callback functions are supposed to handle peripherals interrupts (UART interupt when a character is received, TIMER interupt each second). 

## Timer configuration

To perform periodic data transfer, the BTDevice_timerCallback must be called every second. 

To do this, add the function call in the appropriate TIM_PeriodEllapsed callback (cf example projects).

The corresponding TIMER must be configured to generate interupts every second. You can see TIMER configurations in the MX_TIMx_Init() functions in the example projects.


# Please, send me a message/mail whenever something is not clear or if you're stuck implementing the library.



