STXXL_ROOT      ?= /home/jshun/stxxl-1.3.1
STXXL_CONFIG    ?= stxxl.mk
include $(STXXL_ROOT)/$(STXXL_CONFIG)

# use the variables from stxxl.mk
CXX              = $(STXXL_CXX)
CPPFLAGS        += $(STXXL_CPPFLAGS)
LDLIBS          += $(STXXL_LDLIBS)

CPPFLAGS        += -O3 

ifdef LONG
INTT = -DLONG
endif

PCC = $(CXX)
PCFLAGS = $(CXXFLAGS) $(CPPFLAGS) $(INTT) 
PLFLAGS = $(LDFLAGS)

# required files
GLOBAL_REQUIRE = graph.h
LOCAL_REQUIRE = 
OBJS = TC.o

include ../common/timeRequiredFiles

all: TC testInputs

TC.o: TC.C $(GLOBAL_REQUIRE)
	$(PCC) $(PCFLAGS) -c $< -o $@

TCTime.o: TCTime.C  $(TIME_GLOBAL_REQUIRE) $(TIME_REQUIRE)
	$(PCC) $(PCFLAGS) -c $< -o $@

TC: TCTime.o TC.o
	$(PCC) $(PLFLAGS) -o $@ TCTime.o TC.o $(LDLIBS)

testInputs:
	cp ../common/$@ .

clean:
	rm -f TC *~ *.o
#include MakeBench
