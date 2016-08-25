################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/orderings/Asc.cpp \
../src/orderings/Bfo.cpp \
../src/orderings/Co.cpp \
../src/orderings/Desc.cpp \
../src/orderings/OrderingFactory.cpp \
../src/orderings/Sfo.cpp 

OBJS += \
./src/orderings/Asc.o \
./src/orderings/Bfo.o \
./src/orderings/Co.o \
./src/orderings/Desc.o \
./src/orderings/OrderingFactory.o \
./src/orderings/Sfo.o 

CPP_DEPS += \
./src/orderings/Asc.d \
./src/orderings/Bfo.d \
./src/orderings/Co.d \
./src/orderings/Desc.d \
./src/orderings/OrderingFactory.d \
./src/orderings/Sfo.d 


# Each subdirectory must supply rules for building sources it contributes
src/orderings/%.o: ../src/orderings/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


