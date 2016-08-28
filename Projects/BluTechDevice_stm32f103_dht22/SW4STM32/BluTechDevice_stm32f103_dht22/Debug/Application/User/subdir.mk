################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Application/User/dht22.c \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Src/main.c \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Src/stm32f1xx_hal_msp.c \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Src/stm32f1xx_it.c \
/home/cynako/git/CynaCons/stm32_BluTechDevice/stm32fxxx_hal_BTDevice.c 

OBJS += \
./Application/User/dht22.o \
./Application/User/main.o \
./Application/User/stm32f1xx_hal_msp.o \
./Application/User/stm32f1xx_it.o \
./Application/User/stm32fxxx_hal_BTDevice.o 

C_DEPS += \
./Application/User/dht22.d \
./Application/User/main.d \
./Application/User/stm32f1xx_hal_msp.d \
./Application/User/stm32f1xx_it.d \
./Application/User/stm32fxxx_hal_BTDevice.d 


# Each subdirectory must supply rules for building sources it contributes
Application/User/%.o: ../Application/User/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32F103xE -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Inc" -I/home/cynako/git/CynaCons/stm32_BluTechDevice -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/STM32F1xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/CMSIS/Device/ST/STM32F1xx/Include"  -O3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Application/User/main.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Src/main.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32F103xE -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Inc" -I/home/cynako/git/CynaCons/stm32_BluTechDevice -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/STM32F1xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/CMSIS/Device/ST/STM32F1xx/Include"  -O3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Application/User/stm32f1xx_hal_msp.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Src/stm32f1xx_hal_msp.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32F103xE -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Inc" -I/home/cynako/git/CynaCons/stm32_BluTechDevice -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/STM32F1xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/CMSIS/Device/ST/STM32F1xx/Include"  -O3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Application/User/stm32f1xx_it.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Src/stm32f1xx_it.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32F103xE -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Inc" -I/home/cynako/git/CynaCons/stm32_BluTechDevice -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/STM32F1xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/CMSIS/Device/ST/STM32F1xx/Include"  -O3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

Application/User/stm32fxxx_hal_BTDevice.o: /home/cynako/git/CynaCons/stm32_BluTechDevice/stm32fxxx_hal_BTDevice.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32F103xE -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Inc" -I/home/cynako/git/CynaCons/stm32_BluTechDevice -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/STM32F1xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/CMSIS/Device/ST/STM32F1xx/Include"  -O3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


