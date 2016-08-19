################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_SRCS += \
/home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152/Drivers/CMSIS/Device/ST/STM32L1xx/Source/Templates/gcc/startup_stm32l152xc.s 

OBJS += \
./Application/SW4STM32/startup_stm32l152xc.o 


# Each subdirectory must supply rules for building sources it contributes
Application/SW4STM32/startup_stm32l152xc.o: /home/cynako/Workspaces/openstm32/BluTechDevice_stm32l152/Drivers/CMSIS/Device/ST/STM32L1xx/Source/Templates/gcc/startup_stm32l152xc.s
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Assembler'
	@echo $(PWD)
	arm-none-eabi-as -mcpu=cortex-m3 -mthumb -mfloat-abi=soft -g -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


