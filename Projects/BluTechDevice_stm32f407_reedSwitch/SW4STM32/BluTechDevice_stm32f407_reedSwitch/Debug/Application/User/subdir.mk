################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Src/main.c \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Src/stm32f4_discovery.c \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Src/stm32f4xx_hal_msp.c \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Src/stm32f4xx_it.c \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Src/stm32fxxx_hal_BTDevice.c 

OBJS += \
./Application/User/main.o \
./Application/User/stm32f4_discovery.o \
./Application/User/stm32f4xx_hal_msp.o \
./Application/User/stm32f4xx_it.o \
./Application/User/stm32fxxx_hal_BTDevice.o 

C_DEPS += \
./Application/User/main.d \
./Application/User/stm32f4_discovery.d \
./Application/User/stm32f4xx_hal_msp.d \
./Application/User/stm32f4xx_it.d \
./Application/User/stm32fxxx_hal_BTDevice.d 


# Each subdirectory must supply rules for building sources it contributes
Application/User/main.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Src/main.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32F4DISCOVERY -DSTM32F407xx -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/STM32F4xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/CMSIS/Device/ST/STM32F4xx/Include"  -Os -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Application/User/stm32f4_discovery.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Src/stm32f4_discovery.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32F4DISCOVERY -DSTM32F407xx -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/STM32F4xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/CMSIS/Device/ST/STM32F4xx/Include"  -Os -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Application/User/stm32f4xx_hal_msp.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Src/stm32f4xx_hal_msp.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32F4DISCOVERY -DSTM32F407xx -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/STM32F4xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/CMSIS/Device/ST/STM32F4xx/Include"  -Os -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Application/User/stm32f4xx_it.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Src/stm32f4xx_it.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32F4DISCOVERY -DSTM32F407xx -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/STM32F4xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/CMSIS/Device/ST/STM32F4xx/Include"  -Os -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Application/User/stm32fxxx_hal_BTDevice.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Src/stm32fxxx_hal_BTDevice.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32F4DISCOVERY -DSTM32F407xx -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/STM32F4xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/CMSIS/Device/ST/STM32F4xx/Include"  -Os -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


