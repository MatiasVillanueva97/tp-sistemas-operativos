################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Memoria.c \
../src/funcionesDeCache.c \
../src/funcionesDeTablaInvertida.c \
../src/funcionesManejoDeHeapMemoria.c 

OBJS += \
./src/Memoria.o \
./src/funcionesDeCache.o \
./src/funcionesDeTablaInvertida.o \
./src/funcionesManejoDeHeapMemoria.o 

C_DEPS += \
./src/Memoria.d \
./src/funcionesDeCache.d \
./src/funcionesDeTablaInvertida.d \
./src/funcionesManejoDeHeapMemoria.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


