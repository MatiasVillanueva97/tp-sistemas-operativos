################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/laGranBiblioteca/ProcessControlBlock.c \
../src/laGranBiblioteca/config.c \
../src/laGranBiblioteca/funcionesParaTodosYTodas.c \
../src/laGranBiblioteca/serializarPCB.c \
../src/laGranBiblioteca/sockets.c 

OBJS += \
./src/laGranBiblioteca/ProcessControlBlock.o \
./src/laGranBiblioteca/config.o \
./src/laGranBiblioteca/funcionesParaTodosYTodas.o \
./src/laGranBiblioteca/serializarPCB.o \
./src/laGranBiblioteca/sockets.o 

C_DEPS += \
./src/laGranBiblioteca/ProcessControlBlock.d \
./src/laGranBiblioteca/config.d \
./src/laGranBiblioteca/funcionesParaTodosYTodas.d \
./src/laGranBiblioteca/serializarPCB.d \
./src/laGranBiblioteca/sockets.d 


# Each subdirectory must supply rules for building sources it contributes
src/laGranBiblioteca/%.o: ../src/laGranBiblioteca/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


