################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f417xx.s 

OBJS += \
./Application/SW4STM32/startup_stm32f417xx.o 


# Each subdirectory must supply rules for building sources it contributes
Application/SW4STM32/startup_stm32f417xx.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32f407_reedSwitch/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f417xx.s
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Assembler'
	@echo $(PWD)
	arm-none-eabi-as -mcpu=cortex-m4 -mthumb -mfloat-abi=hard -mfpu=fpv4-sp-d16 -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


