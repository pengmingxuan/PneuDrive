################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/CMSIS/DSP/Source/CommonTables/arm_common_tables.c \
../Drivers/CMSIS/DSP/Source/CommonTables/arm_const_structs.c 

OBJS += \
./Drivers/CMSIS/DSP/Source/CommonTables/arm_common_tables.o \
./Drivers/CMSIS/DSP/Source/CommonTables/arm_const_structs.o 

C_DEPS += \
./Drivers/CMSIS/DSP/Source/CommonTables/arm_common_tables.d \
./Drivers/CMSIS/DSP/Source/CommonTables/arm_const_structs.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/CMSIS/DSP/Source/CommonTables/%.o: ../Drivers/CMSIS/DSP/Source/CommonTables/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: MCU GCC Compiler'
	@echo $(PWD)
	arm-none-eabi-gcc -mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16 '-D__weak=__attribute__((weak))' '-D__FPU_PRESENT=1U' -DARM_MATH_CM7 -DCIFX_TOOLKIT_HWIF '-D__packed=__attribute__((__packed__))' -DUSE_HAL_DRIVER -DSTM32F767xx -I"C:/Users/40207/OneDrive/OneDriveDocumentation/STM32WorkSpaceOnedrive/PneuDrive/Inc" -I"C:/Users/40207/OneDrive/OneDriveDocumentation/STM32WorkSpaceOnedrive/PneuDrive/Src/framework/frameInc" -I"C:/Users/40207/OneDrive/OneDriveDocumentation/STM32WorkSpaceOnedrive/PneuDrive/User" -I"C:/Users/40207/OneDrive/OneDriveDocumentation/STM32WorkSpaceOnedrive/PneuDrive/Drivers/CMSIS/DSP/Include" -I"C:/Users/40207/OneDrive/OneDriveDocumentation/STM32WorkSpaceOnedrive/PneuDrive/Drivers/STM32F7xx_HAL_Driver/Inc" -I"C:/Users/40207/OneDrive/OneDriveDocumentation/STM32WorkSpaceOnedrive/PneuDrive/Drivers/STM32F7xx_HAL_Driver/Inc/Legacy" -I"C:/Users/40207/OneDrive/OneDriveDocumentation/STM32WorkSpaceOnedrive/PneuDrive/Middlewares/Third_Party/FreeRTOS/Source/include" -I"C:/Users/40207/OneDrive/OneDriveDocumentation/STM32WorkSpaceOnedrive/PneuDrive/Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS" -I"C:/Users/40207/OneDrive/OneDriveDocumentation/STM32WorkSpaceOnedrive/PneuDrive/Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM7/r0p1" -I"C:/Users/40207/OneDrive/OneDriveDocumentation/STM32WorkSpaceOnedrive/PneuDrive/Drivers/CMSIS/Device/ST/STM32F7xx/Include" -I"C:/Users/40207/OneDrive/OneDriveDocumentation/STM32WorkSpaceOnedrive/PneuDrive/Drivers/CMSIS/Include"  -O3 -g3 -Wall -fmessage-length=0 -ffunction-sections -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


