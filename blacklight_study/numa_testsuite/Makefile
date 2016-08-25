CPPFLAGS =   -g  -pthread
#-lpthread
#CPP = g++
CPP = icpc

all: numatest

numatest: main.cpp utils.cpp task_worker.cpp pthread_tools.cpp memory_access_tests.cpp 
	$(CPP) main.cpp pthread_tools.cpp  utils.cpp task_worker.cpp memory_access_tests.cpp -o numatest $(CPPFLAGS)


