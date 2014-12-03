CC     = gcc
CXX    = g++


include ../../fifo_dev/buffer.makefile

DIRINCS = $(RINGBUFFERDIR) 

ifneq ($(shell uname -s), Darwin)
RT = -lrt
STATIC = -static -static-libgcc -static-libstdc++
endif

CFLAGS =     -O1  -Wall -std=c99	  -DRDTSCP=1 
CXXFLAGS =   -O1  -Wall -std=c++11 -DRDTSCP=1  


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
	$(CXX) $(CXXFLAGS) $(STATIC) $(INCS) $(OBJS) $(LIBS) -o $(EXE)

%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $(CPPFLAGS) $(INCS) -o $@ $<

%.o: %.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(INCS) -o $@ $<

.PHONY: clean
clean:
	rm -rf $(EXE) $(OBJS)
