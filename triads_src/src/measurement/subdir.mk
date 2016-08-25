################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/measurement/StopWatch.cpp 

OBJS += \
./src/measurement/StopWatch.o 

CPP_DEPS += \
./src/measurement/StopWatch.d 


# Each subdirectory must supply rules for building sources it contributes
src/measurement/%.o: ../src/measurement/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


