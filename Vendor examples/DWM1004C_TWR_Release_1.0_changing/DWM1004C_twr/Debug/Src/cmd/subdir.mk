################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/cmd/cmd.c \
../Src/cmd/cmd_fn.c \
../Src/cmd/cmd_uart_rx.c 

OBJS += \
./Src/cmd/cmd.o \
./Src/cmd/cmd_fn.o \
./Src/cmd/cmd_uart_rx.o 

C_DEPS += \
./Src/cmd/cmd.d \
./Src/cmd/cmd_fn.d \
./Src/cmd/cmd_uart_rx.d 


# Each subdirectory must supply rules for building sources it contributes
Src/cmd/%.o Src/cmd/%.su Src/cmd/%.cyclo: ../Src/cmd/%.c Src/cmd/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0plus -std=gnu11 -g3 -DUSE_FULL_LL_DRIVER -DHSE_VALUE=8000000 -DHSE_STARTUP_TIMEOUT=100 -DLSE_STARTUP_TIMEOUT=5000 -DLSE_VALUE=32768 -DMSI_VALUE=131072 -DHSI_VALUE=16000000 -DLSI_VALUE=37000 -DVDD_VALUE=3300 -DPREFETCH_ENABLE=0 -DINSTRUCTION_CACHE_ENABLE=1 -DDATA_CACHE_ENABLE=1 '-D__weak=__attribute__((weak))' '-D__packed=__attribute__((__packed__))' -DSTM32L041xx -c -I../Inc -I../Src -I../Src/instance -I../Src/mems -I../Src/Port -I../Src/utils -I../Src/cmd -I../Src/config -I../Drivers/deca_driver -I../Drivers/STM32L0xx_HAL_Driver/Inc -I../Drivers/CMSIS/Device/ST/STM32L0xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Src-2f-cmd

clean-Src-2f-cmd:
	-$(RM) ./Src/cmd/cmd.cyclo ./Src/cmd/cmd.d ./Src/cmd/cmd.o ./Src/cmd/cmd.su ./Src/cmd/cmd_fn.cyclo ./Src/cmd/cmd_fn.d ./Src/cmd/cmd_fn.o ./Src/cmd/cmd_fn.su ./Src/cmd/cmd_uart_rx.cyclo ./Src/cmd/cmd_uart_rx.d ./Src/cmd/cmd_uart_rx.o ./Src/cmd/cmd_uart_rx.su

.PHONY: clean-Src-2f-cmd

