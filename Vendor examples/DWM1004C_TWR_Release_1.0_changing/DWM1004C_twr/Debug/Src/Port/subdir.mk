################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/Port/deca_sleep.c \
../Src/Port/deca_spi.c \
../Src/Port/deca_uart.c \
../Src/Port/port_platform.c 

OBJS += \
./Src/Port/deca_sleep.o \
./Src/Port/deca_spi.o \
./Src/Port/deca_uart.o \
./Src/Port/port_platform.o 

C_DEPS += \
./Src/Port/deca_sleep.d \
./Src/Port/deca_spi.d \
./Src/Port/deca_uart.d \
./Src/Port/port_platform.d 


# Each subdirectory must supply rules for building sources it contributes
Src/Port/%.o Src/Port/%.su Src/Port/%.cyclo: ../Src/Port/%.c Src/Port/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DUSE_FULL_LL_DRIVER -DHSE_VALUE=8000000 -DHSE_STARTUP_TIMEOUT=100 -DLSE_STARTUP_TIMEOUT=5000 -DLSE_VALUE=32768 -DMSI_VALUE=131072 -DHSI_VALUE=16000000 -DLSI_VALUE=37000 -DVDD_VALUE=3300 -DPREFETCH_ENABLE=0 -DINSTRUCTION_CACHE_ENABLE=1 -DDATA_CACHE_ENABLE=1 '-D__weak=__attribute__((weak))' '-D__packed=__attribute__((__packed__))' -DSTM32L041xx -c -I../Inc -I../Src -I../Src/instance -I../Src/mems -I../Src/Port -I../Src/utils -I../Src/cmd -I../Src/config -I../Drivers/deca_driver -I../Drivers/STM32L0xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32L0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Src-2f-Port

clean-Src-2f-Port:
	-$(RM) ./Src/Port/deca_sleep.cyclo ./Src/Port/deca_sleep.d ./Src/Port/deca_sleep.o ./Src/Port/deca_sleep.su ./Src/Port/deca_spi.cyclo ./Src/Port/deca_spi.d ./Src/Port/deca_spi.o ./Src/Port/deca_spi.su ./Src/Port/deca_uart.cyclo ./Src/Port/deca_uart.d ./Src/Port/deca_uart.o ./Src/Port/deca_uart.su ./Src/Port/port_platform.cyclo ./Src/Port/port_platform.d ./Src/Port/port_platform.o ./Src/Port/port_platform.su

.PHONY: clean-Src-2f-Port

