TARGET = ht_adaptor.so
OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
CPPFLAGS+= -g -shared -Wall -fPIC -finline-functions -fexceptions -std=c++11 -I ../../core -I../
LDFLAGS+= -L ./ -lHtAPIClient -L../mysql -lmysqlclient -lcrypto -lssl

all: ${OBJECTS}
	echo ${OBJECTS}
	g++ ${OBJECTS} -o ${TARGET} $(CPPFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f ${OBJECTS} ${TARGET}