################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/algorithms/thesis/AbstractOrientedTriadAlgorithm.cpp \
../src/algorithms/thesis/DegreeStatistic.cpp \
../src/algorithms/thesis/LO1.cpp \
../src/algorithms/thesis/LOM.cpp \
../src/algorithms/thesis/LON.cpp \
../src/algorithms/thesis/SO1.cpp \
../src/algorithms/thesis/SOM.cpp \
../src/algorithms/thesis/SON.cpp \
../src/algorithms/thesis/SONless.cpp \
../src/algorithms/thesis/T1O1.cpp \
../src/algorithms/thesis/T1OM.cpp \
../src/algorithms/thesis/T2O1.cpp \
../src/algorithms/thesis/T2OM.cpp 

OBJS += \
./src/algorithms/thesis/AbstractOrientedTriadAlgorithm.o \
./src/algorithms/thesis/DegreeStatistic.o \
./src/algorithms/thesis/LO1.o \
./src/algorithms/thesis/LOM.o \
./src/algorithms/thesis/LON.o \
./src/algorithms/thesis/SO1.o \
./src/algorithms/thesis/SOM.o \
./src/algorithms/thesis/SON.o \
./src/algorithms/thesis/SONless.o \
./src/algorithms/thesis/T1O1.o \
./src/algorithms/thesis/T1OM.o \
./src/algorithms/thesis/T2O1.o \
./src/algorithms/thesis/T2OM.o 

CPP_DEPS += \
./src/algorithms/thesis/AbstractOrientedTriadAlgorithm.d \
./src/algorithms/thesis/DegreeStatistic.d \
./src/algorithms/thesis/LO1.d \
./src/algorithms/thesis/LOM.d \
./src/algorithms/thesis/LON.d \
./src/algorithms/thesis/SO1.d \
./src/algorithms/thesis/SOM.d \
./src/algorithms/thesis/SON.d \
./src/algorithms/thesis/SONless.d \
./src/algorithms/thesis/T1O1.d \
./src/algorithms/thesis/T1OM.d \
./src/algorithms/thesis/T2O1.d \
./src/algorithms/thesis/T2OM.d 


# Each subdirectory must supply rules for building sources it contributes
src/algorithms/thesis/%.o: ../src/algorithms/thesis/%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '

