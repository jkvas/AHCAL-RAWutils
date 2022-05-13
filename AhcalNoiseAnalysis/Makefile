TARGET = AhcalNoiseAnalysis
CXX=g++
RM=rm -f
CPPFLAGS=-std=c++11 -O2 -Wall
#LDFLAGS=-static
LDLIBS=

SRCS=$(wildcard *.cpp)
OBJS=$(subst .cpp,.o,$(SRCS))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS) 

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)
	$(RM) $(TARGET)

distclean: clean
	$(RM) *~ .depend

include .depend
 
