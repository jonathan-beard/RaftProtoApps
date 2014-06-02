CC    = gcc
CXX   = g++

CFLAGS =    -O2 -Wall -std=c99
CXXFLAGS =  -O2 -Wall -std=c++11

COBJS =   getrandom
CXXOBJS = main pointer shm  

CFILES = $(addsuffix .c, $(COBJS) )
CXXFILES = $(addsuffix .cpp, $(CXXOBJS) )

OBJS = $(addsuffix .o, $(COBJS) ) $(addsuffix .o, $(CXXOBJS) )

INCS = 
LIBS = -lpthread #-lrt

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
