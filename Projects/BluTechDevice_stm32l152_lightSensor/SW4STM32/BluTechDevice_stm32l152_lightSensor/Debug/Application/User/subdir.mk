################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Src/buzzer.c \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Src/main.c \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Src/stm32fxxx_hal_BTDevice.c \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Src/stm32l152c_discovery.c \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Src/stm32l1xx_hal_msp.c \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Src/stm32l1xx_it.c 

OBJS += \
./Application/User/buzzer.o \
./Application/User/main.o \
./Application/User/stm32fxxx_hal_BTDevice.o \
./Application/User/stm32l152c_discovery.o \
./Application/User/stm32l1xx_hal_msp.o \
./Application/User/stm32l1xx_it.o 

C_DEPS += \
./Application/User/buzzer.d \
./Application/User/main.d \
./Application/User/stm32fxxx_hal_BTDevice.d \
./Application/User/stm32l152c_discovery.d \
./Application/User/stm32l1xx_hal_msp.d \
./Application/User/stm32l1xx_it.d 


# Each subdirectory must supply rules for building sources it contributes
Application/User/buzzer.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Src/buzzer.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32L152xC -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/STM32L1xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/STM32L1xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/CMSIS/Device/ST/STM32L1xx/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Inc"  -Os -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Application/User/main.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Src/main.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32L152xC -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/STM32L1xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/STM32L1xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/CMSIS/Device/ST/STM32L1xx/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Inc"  -Os -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Application/User/stm32fxxx_hal_BTDevice.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Src/stm32fxxx_hal_BTDevice.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32L152xC -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/STM32L1xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/STM32L1xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/CMSIS/Device/ST/STM32L1xx/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Inc"  -Os -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Application/User/stm32l152c_discovery.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Src/stm32l152c_discovery.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32L152xC -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/STM32L1xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/STM32L1xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/CMSIS/Device/ST/STM32L1xx/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Inc"  -Os -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Application/User/stm32l1xx_hal_msp.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Src/stm32l1xx_hal_msp.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32L152xC -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/STM32L1xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/STM32L1xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/CMSIS/Device/ST/STM32L1xx/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Inc"  -Os -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Application/User/stm32l1xx_it.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Src/stm32l1xx_it.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32L152xC -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/STM32L1xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/STM32L1xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Drivers/CMSIS/Device/ST/STM32L1xx/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152_lightSensor/Inc"  -Os -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


