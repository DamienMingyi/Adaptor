TARGET = fix_adaptor
OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
CPPFLAGS+=  -O3   -Wall  -finline-functions -fexceptions -std=c++11 -I ../../core -I../ -I/usr/local/include/
LDFLAGS+= -ldl  -L/usr/local/lib  -lquickfix -lhiredis -L../modules -lbsim_core

all: ${OBJECTS}
	echo ${OBJECTS}
	g++  ${OBJECTS}  $(CPPFLAGS) $(LDFLAGS) -o ${TARGET}

.PHONY: clean
clean:
	rm -f ${OBJECTS} ${TARGET}
