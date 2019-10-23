TARGET = jt_adaptor.so
OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
CPPFLAGS+= -g -D OS_IS_LINUX -shared -Wall -fPIC -finline-functions -fexceptions -std=c++11 -I ../../core -I../ -I ./include/ -I./include/c++
LDFLAGS+=  -L ./lib -lmaTradeApi -lmathirdparty

all: ${OBJECTS}
	echo ${OBJECTS}
	g++ ${OBJECTS} -o ${TARGET} $(CPPFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f ${OBJECTS} ${TARGET}

