################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/network/DLGraph.cpp \
../src/network/Graph.cpp \
../src/network/IOGraph.cpp 

OBJS += \
./src/network/DLGraph.o \
./src/network/Graph.o \
./src/network/IOGraph.o 

CPP_DEPS += \
./src/network/DLGraph.d \
./src/network/Graph.d \
./src/network/IOGraph.d 


# Each subdirectory must supply rules for building sources it contributes
src/network/%.o: ../src/network/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


