################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/deca_driver/deca_device.c \
../Drivers/deca_driver/deca_params_init.c 

OBJS += \
./Drivers/deca_driver/deca_device.o \
./Drivers/deca_driver/deca_params_init.o 

C_DEPS += \
./Drivers/deca_driver/deca_device.d \
./Drivers/deca_driver/deca_params_init.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/deca_driver/%.o Drivers/deca_driver/%.su Drivers/deca_driver/%.cyclo: ../Drivers/deca_driver/%.c Drivers/deca_driver/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DUSE_FULL_LL_DRIVER -DHSE_VALUE=8000000 -DHSE_STARTUP_TIMEOUT=100 -DLSE_STARTUP_TIMEOUT=5000 -DLSE_VALUE=32768 -DMSI_VALUE=131072 -DHSI_VALUE=16000000 -DLSI_VALUE=37000 -DVDD_VALUE=3300 -DPREFETCH_ENABLE=0 -DINSTRUCTION_CACHE_ENABLE=1 -DDATA_CACHE_ENABLE=1 '-D__weak=__attribute__((weak))' '-D__packed=__attribute__((__packed__))' -DSTM32L041xx -c -I../Inc -I../Src -I../Src/instance -I../Src/mems -I../Src/Port -I../Src/utils -I../Src/cmd -I../Src/config -I../Drivers/deca_driver -I../Drivers/STM32L0xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32L0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-deca_driver

clean-Drivers-2f-deca_driver:
	-$(RM) ./Drivers/deca_driver/deca_device.cyclo ./Drivers/deca_driver/deca_device.d ./Drivers/deca_driver/deca_device.o ./Drivers/deca_driver/deca_device.su ./Drivers/deca_driver/deca_params_init.cyclo ./Drivers/deca_driver/deca_params_init.d ./Drivers/deca_driver/deca_params_init.o ./Drivers/deca_driver/deca_params_init.su

.PHONY: clean-Drivers-2f-deca_driver

