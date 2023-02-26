all: gbr2ngc

INSTALLDIR=/usr/local/bin

COMMON-OBJS += gerber_interpreter.o
COMMON-OBJS += string_ll.o

GBR2NGC-OBJS += $(COMMON-OBJS)
GBR2NGC-OBJS += clipper.o
GBR2NGC-OBJS += gbr2ngc.o
GBR2NGC-OBJS += gbr2ngc_aperture.o
GBR2NGC-OBJS += gbr2ngc_construct.o
GBR2NGC-OBJS += gbr2ngc_export.o
GBR2NGC-OBJS += gbr2ngc_globals.o
GBR2NGC-OBJS += tesexpr.o

TEST-OBJS += $(COMMON-OBJS)
TEST-OBJS += test_gerber_interpreter.o

CCOPT=
#CCOPT=-O2

CC=gcc
CFLAGS=-g -Wall $(CCOPT)

CXX=g++
CXXFLAGS=-g --std=c++11 -Wall $(CCOPT)

#LDFLAGS=-lstdc++ -lboost_thread -lm

%.o: %.c
	$(CC) -o $@ $(CFLAGS) -c $<

%.o: %.cpp
	$(CXX) -o $@ $(CXXFLAGS) -c $<

gbr2ngc: $(GBR2NGC-OBJS)
	$(CXX) -o $@ $(CXXFLAGS) $(GBR2NGC-OBJS) $(LDFLAGS)

test_gerber_interpreter: $(TEST-OBJS)
	$(CXX) -o $@ $(CXXFLAGS) $(TEST-OBJS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f *.o gbr2ngc
	rm -f *.o test_gerber_interpreter

.PHONY: install
install: gbr2ngc
	mkdir -p $(INSTALLDIR)
	cp $< $(INSTALLDIR)
