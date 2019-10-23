TARGET = hfs_smart_router.so
OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
CPPFLAGS+= -g -shared -Wall -fPIC -finline-functions -fexceptions -std=c++11 -I ../../core -I../ -I/usr/local/instantclient_12_1/sdk/include 
LDFLAGS+= -L../modules -L/usr/local/instantclient_12_1 -lclntsh -locci -lhiredis

all: ${OBJECTS}
	echo ${OBJECTS}
	g++ ${OBJECTS} -o ${TARGET} $(CPPFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f ${OBJECTS} ${TARGET}

