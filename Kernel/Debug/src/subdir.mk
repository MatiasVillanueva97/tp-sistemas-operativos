################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/Kernel.c \
../src/funcionesCPU.c \
../src/funcionesCapaFS.c \
../src/funcionesConsola.c \
../src/funcionesConsolaKernel.c \
../src/funcionesHeap.c \
../src/funcionesMemoria.c \
../src/funcionesPCB.c \
../src/funcionesSemaforosYCompartidas.c \
../src/iNotify.c 

OBJS += \
./src/Kernel.o \
./src/funcionesCPU.o \
./src/funcionesCapaFS.o \
./src/funcionesConsola.o \
./src/funcionesConsolaKernel.o \
./src/funcionesHeap.o \
./src/funcionesMemoria.o \
./src/funcionesPCB.o \
./src/funcionesSemaforosYCompartidas.o \
./src/iNotify.o 

C_DEPS += \
./src/Kernel.d \
./src/funcionesCPU.d \
./src/funcionesCapaFS.d \
./src/funcionesConsola.d \
./src/funcionesConsolaKernel.d \
./src/funcionesHeap.d \
./src/funcionesMemoria.d \
./src/funcionesPCB.d \
./src/funcionesSemaforosYCompartidas.d \
./src/iNotify.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: Cross GCC Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


