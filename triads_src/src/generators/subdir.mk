################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/generators/AbstractGenerator.cpp \
../src/generators/PrefAttTriaClosure.cpp \
../src/generators/TorusGNP.cpp 

OBJS += \
./src/generators/AbstractGenerator.o \
./src/generators/PrefAttTriaClosure.o \
./src/generators/TorusGNP.o 

CPP_DEPS += \
./src/generators/AbstractGenerator.d \
./src/generators/PrefAttTriaClosure.d \
./src/generators/TorusGNP.d 


# Each subdirectory must supply rules for building sources it contributes
src/generators/%.o: ../src/generators/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


