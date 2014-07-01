CC     ?= gcc
CXX    ?= g++


include ../../simpleringbuffer/buffer.makefile

DIRINCS = $(RINGBUFFERDIR) 

ifneq ($(shell uname -s), Darwin)
RT = -lrt
#STATIC = -static -static-libgcc -static-libstdc++
endif

CFLAGS =     -O3 -msse4.1 -Wall -std=c99	  -DRDTSCP=1 
CXXFLAGS =   -O3 -msse4.1 -Wall -std=c++11 -DRDTSCP=1  

BUFFERCPPOBJS = pointer shm
BUFFERCOBJS   = getrandom


COBJS =  $(RBCOBJS) 
CXXOBJS = main $(RBCXXOBJS) 

CFILES = $(addsuffix .c, $(COBJS))
CXXFILES = $(addsuffix .cpp, $(CXXOBJS))

OBJS = $(addsuffix .o, $(COBJS) ) $(addsuffix .o, $(CXXOBJS) )




INCS = $(addprefix -I, $(DIRINCS))
LIBS = -lpthread $(RT)

EXE  = sgrep

compile: $(CXXFILES) $(CFILES)
	$(MAKE) $(OBJS)
	$(CXX) $(CXXFLAGS) $(STATIC) $(INCS) $(OBJS) $(LIBS) -o $(EXE)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $(INCS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INCS) -o $@ $<

.PHONY: clean
clean:
	rm -rf $(EXE) $(OBJS)
