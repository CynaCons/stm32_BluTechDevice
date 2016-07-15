# stm32_BluTechDevice
Drivers to command the BluTech devices using an STM32 device

                                                                        The user !
                                                                          XXXXX
                                          +display commands menu          XXXXX
                                          +display values which are sent  XXXXX
    +-------+ sensor data+------------+                                     X       You can plug to the device to input
    |       |          |              | ---------------------------->       X       some settings or check if it's working
    | sensor+---------->              |                                 XXXXXXXXXX
    |       |          |              |                                     X       Once the settings are finished,the
    +-------+          |   STM32Fxxx  |        user uart                    X       devicewill work on it's own.
                       |              |                                     X
                       |              |                                    XXX
                       |              | ----------------------------+    XXX XXX
                       +--------------+                                XXX     XXX
                                              +perform commands (like
                                      ^        network join)
                       +              |       +input settings (timer period) 
                       |              |        +send sample data & other
                       |              |
                       |              |
                       |    device    |
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


I highly recommend to use an STM32CubeMX generated project with the HAL library.

You will find details in the example files (main.c, stm32fxxx_it.c)


Once it's all complete, load the program, plug your user uart and start the board. You should see this menu ! 


      "******* Application Menu *********\r\n",
      			"==================================\r\n",
      			"\r\n",
      			"Application Control\r\n",
      			"    menu : display this menu\r\n",
      			"    rs   : to reset the input buffer (type this if you made a typing mistake\r\n",
      			"    get sensor value : to display the current sensor value\r\n",
      			"    set automode on  : to start sending data periodically\r\n",
      			"    set automode off : to start sending data periodically\r\n",
      			"    send data        : to send data to the device just once\r\n",
      			"\r\n",
      			"BluTech Device Control\r\n",
      			"    rf signal check  : perform a signal check\r\n",
      			"    network join     : join the Gateway network\r\n",
      			"    send sample data : to send a sample of data for testing\r\n",
      			"\r\n",
      			"Systick Control\r\n",
      			"    set timer period : to set the value (in seconds) for the periodical timer period\r\n",
      			"    get timer period : to get the current value for periodical timer period\r\n",
      			"\r\n",
      			"WARNING : You have to join a network in order to send data though the RF module \r\n"
