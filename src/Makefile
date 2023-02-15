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
GBR2NGC-OBJS += gbr2ngc_heightmap.o
GBR2NGC-OBJS += gbr2ngc_profile.o
GBR2NGC-OBJS += tesexpr.o

TEST-OBJS += $(COMMON-OBJS)
TEST-OBJS += test_gerber_interpreter.o

CC=gcc
COPT=-g
COPT2=-g -DGERBER_TEST

#COPT=-O3
#COPT2=-O3 -DGERBER_TEST

CXX=g++
CXXOPT=-g --std=c++11
#CXXOPT=-O3 --std=c++11
CXXLIB=-lstdc++ -lboost_thread -lm

%.o: %.c
	$(CC) -o $@ $(COPT) -c $<

%.o: %.cpp
	$(CXX) -o $@ $(CXXOPT) -c $<

gbr2ngc: $(GBR2NGC-OBJS)
	$(CXX) -o $@ $(CXXOPT) $(GBR2NGC-OBJS) $(CXXLIBS)

test_gerber_interpreter: $(TEST-OBJS)
	$(CXX) -o $@ $(CXXOPT) $(TEST-OBJS) $(CXXLIBS)

.PHONY: clean
clean:
	rm -f *.o gbr2ngc
	rm -f *.o test_gerber_interpreter

.PHONY: install
install: gbr2ngc
	mkdir -p $(INSTALLDIR)
	cp $< $(INSTALLDIR)
