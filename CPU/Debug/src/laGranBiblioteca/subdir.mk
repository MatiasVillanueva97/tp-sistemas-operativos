################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/laGranBiblioteca/config.c \
../src/laGranBiblioteca/sockets.c 

OBJS += \
./src/laGranBiblioteca/config.o \
./src/laGranBiblioteca/sockets.o 

C_DEPS += \
./src/laGranBiblioteca/config.d \
./src/laGranBiblioteca/sockets.d 


# Each subdirectory must supply rules for building sources it contributes
src/laGranBiblioteca/%.o: ../src/laGranBiblioteca/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


