CPPSRCS		= main.cpp cpp_framework.cpp
TARGET		= test_intel64

CPP			= g++

CPPFLAGS	= -lcilkrts -fcilkplus -O2 -m64 -DNDEBUG -DINTEL64 -DCILKP
LFLAGS		= -lcilkrts -fcilkplus -O2 -m64 -DNDEBUG -DINTEL64 -DCILKP

OBJS		= $(CPPSRCS:.cpp=.o)

all: $(TARGET)

main.o: ./test/main.cpp
	$(CPP) $(CPPFLAGS) -c ./test/main.cpp

cpp_framework.o: ./framework/cpp_framework.cpp
	$(CPP) $(CPPFLAGS) -c ./framework/cpp_framework.cpp

$(TARGET): $(OBJS)
	$(CPP) $(LFLAGS) $(OBJS) -o $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)

depend:
	mkdep $(SRCS)
