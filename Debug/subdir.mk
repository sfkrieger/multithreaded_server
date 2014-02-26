################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../AcceptTCPConnection.c \
../CreateTCPServerSocket.c \
../DieWithError.c \
../HandleTCPClient.c \
../mtserver.c 

OBJS += \
./AcceptTCPConnection.o \
./CreateTCPServerSocket.o \
./DieWithError.o \
./HandleTCPClient.o \
./mtserver.o 

C_DEPS += \
./AcceptTCPConnection.d \
./CreateTCPServerSocket.d \
./DieWithError.d \
./HandleTCPClient.d \
./mtserver.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o"$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


