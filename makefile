
TARGET = ExchGateway.exe
OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
CPPFLAGS+= -g -Wall -fPIC -finline-functions -fexceptions -std=c++11 -I../core -I./include -I/usr/local/instantclient_12_1/sdk/include  -I/usr/local/include 
LDFLAGS+= -ldl -pthread -rdynamic -lbsim_core -L ../bin/alphacode -L./modules -L/usr/lib64 -L /usr/local/lib/ -lboost_system -lboost_filesystem -L/usr/local/instantclient_12_1 -lclntsh -locci -lboost_thread -lboost_log -lboost_regex -lboost_program_options  -lboost_serialization 

all: ${OBJECTS}
	echo ${OBJECTS}
	g++ ${OBJECTS} -o ./${TARGET} $(CPPFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f ${OBJECTS} ./${TARGET}
