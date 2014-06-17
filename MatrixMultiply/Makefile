CC    = gcc
CXX   = g++


include ../../simpleringbuffer/buffer.makefile

DIRINCS = $(RINGBUFFERDIR) 

ifneq ($(shell uname -s), Darwin)
RT = -lrt
endif

CFLAGS =     -O0 -g -Wall -std=c99	 -DAMD=1 
CXXFLAGS =   -O0 -g -Wall -std=c++11 -DAMD=1 

BUFFERCPPOBJS = pointer shm
BUFFERCOBJS   = getrandom


COBJS =  $(RBCOBJS) 
CXXOBJS = main $(RBCXXOBJS) 

CFILES = $(addsuffix .c, $(COBJS))
CXXFILES = $(addsuffix .cpp, $(CXXOBJS))

OBJS = $(addsuffix .o, $(COBJS) ) $(addsuffix .o, $(CXXOBJS) )




INCS = $(addprefix -I, $(DIRINCS))
LIBS = -lpthread $(RT)

EXE  = mmult

compile: $(CXXFILES) $(CFILES)
	$(MAKE) $(OBJS)
	$(CXX) $(CXXFLAGS) $(INCS) $(OBJS) $(LIBS) -o $(EXE)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(INCS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) $(INCS) -o $@ $<

.PHONY: clean
clean:
	rm -rf $(EXE) $(OBJS)
