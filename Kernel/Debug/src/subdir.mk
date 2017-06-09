################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Kernel.c \
../src/funcionesCPU.c \
../src/funcionesConsola.c \
../src/funcionesConsolaKernel.c \
../src/funcionesMemoria.c \
../src/funcionesPCB.c \
../src/funcionesSemaforos.c 

OBJS += \
./src/Kernel.o \
./src/funcionesCPU.o \
./src/funcionesConsola.o \
./src/funcionesConsolaKernel.o \
./src/funcionesMemoria.o \
./src/funcionesPCB.o \
./src/funcionesSemaforos.o 

C_DEPS += \
./src/Kernel.d \
./src/funcionesCPU.d \
./src/funcionesConsola.d \
./src/funcionesConsolaKernel.d \
./src/funcionesMemoria.d \
./src/funcionesPCB.d \
./src/funcionesSemaforos.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


