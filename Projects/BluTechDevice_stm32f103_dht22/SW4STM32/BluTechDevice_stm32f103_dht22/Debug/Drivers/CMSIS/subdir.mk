################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c 

OBJS += \
./Drivers/CMSIS/system_stm32f1xx.o 

C_DEPS += \
./Drivers/CMSIS/system_stm32f1xx.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/CMSIS/system_stm32f1xx.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -D__weak="__attribute__((weak))" -D__packed="__attribute__((__packed__))" -DUSE_HAL_DRIVER -DSTM32F103xE -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Inc" -I/home/cynako/git/CynaCons/stm32_BluTechDevice -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/STM32F1xx_HAL_Driver/Inc" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/CMSIS/Include" -I"/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f103_dht22/Drivers/CMSIS/Device/ST/STM32F1xx/Include"  -O3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

