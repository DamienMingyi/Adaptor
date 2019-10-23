TARGET = fix_adaptor.so
OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
CPPFLAGS+= -O3 -shared -Wall -fPIC -finline-functions -fexceptions -std=c++11 -I ../../core -I../ -I/usr/local/include/
LDFLAGS+= -L/usr/local/lib  -lquickfix

all: ${OBJECTS}
	echo ${OBJECTS}
	g++ ${OBJECTS} -o ${TARGET} $(CPPFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f ${OBJECTS} ${TARGET}