OBJS=Application.o Config.o Soap.o Server.o ServiceStruct.o
CFLAGS=
CPPFLAGS=-Wno-deprecated

all: xmlws

clean:
	rm *.o xmlws

xmlws: main.o ezxml.o $(OBJS)
	g++ main.o ezxml.o $(OBJS) -o xmlws -lpthread

main.o: main.cc
	g++ -c main.cc -o main.o ${CPPFLAGS}

ezxml.o: ezxml.c ezxml.h
	gcc -c ezxml.c -o ezxml.o ${CFLAGS}

Application.o: Application.cc Application.hh
	g++ -c Application.cc -o Application.o ${CPPFLAGS}

Config.o: Config.cc Config.hh
	g++ -c Config.cc -o Config.o ${CPPFLAGS}

Server.o: Server.cc Server.hh
	g++ -c Server.cc -o Server.o ${CPPFLAGS}

ServiceStruct.o: ServiceStruct.cc ServiceStruct.hh
	g++ -c ServiceStruct.cc -o ServiceStruct.o ${CPPFLAGS}

Soap.o: Soap.cc Soap.hh
	g++ -c Soap.cc -o Soap.o ${CPPFLAGS}
