CC=g++
CFLAGS=-c -Wall
SOURCES=src/SocketServer.cc src/SocketPoll.cc src/main.cc

ROOT=..
LIBS_PATH=-L$(ROOT)/lib
LIBS=-lxthread -lsysutil -lmisc -lpthread

INCLUDE=-I./
INCLUDE+=-I$(ROOT)

OBJECTS=$(SOURCES:.cc=.o)

EXECUTABLE=server

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(LIBS_PATH) $(LIBS) -o $@

.cc.o:
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

clean:
	rm src/*.o $(EXECUTABLE)

