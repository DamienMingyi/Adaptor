TARGET = ctpse_adaptor.so
OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp)) 
CPPFLAGS+= -g -shared -Wall -fPIC -finline-functions -fexceptions -std=c++11 -I ../../core -I../ -I ./CTP_SE_15
LDFLAGS+= -L ./CTP_SE_15 -lthosttraderapi_se -LLinuxDataCollect.so 

all: ${OBJECTS}
	echo ${OBJECTS}
	g++ ${OBJECTS} -o ${TARGET} $(CPPFLAGS) $(LDFLAGS)

.PHONY: clean
clean:
	rm -f ${OBJECTS} ${TARGET}

