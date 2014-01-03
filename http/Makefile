CC=g++
CFLAGS=-c -Wall -g
SOURCES=main.cc HttpClient.cc HttpBuffer.cc HttpServer.cc SocketServer.cc SocketPoll.cc

ROOT=../
LIBS_PATH=-L$(ROOT)/lib
LIBS=-lxthread -lsysutil -lxthread -lmisc -lpthread -lrt

INCLUDE=-I./
INCLUDE+=-I$(ROOT)

OBJECTS=$(SOURCES:.cc=.o)

EXE=http_server

all: $(SOURCES) $(EXE)

$(EXE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(INCLUDE) $(LIBS_PATH) $(LIBS) -o $@

.cc.o:
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

clean:
	rm *.o $(EXE)

