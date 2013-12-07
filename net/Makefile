CC=g++
CFLAGS=-c -Wall -g

RES=../lib
ROOT=..
LIBS_PATH=-L$(ROOT)/lib
LIBS=-lxthread -lsysutil -lmisc -lpthread -lrt

INCLUDE=-I./
INCLUDE+=-I$(ROOT)

SOURCES=src/SocketServer.cc src/SocketPoll.cc
OBJECTS=$(SOURCES:.cc=.o)

LIBNAME=libxnet.a

all: $(SOURCES) $(LIBNAME)

$(RES):
	mkdir -p $(RES)

$(LIBNAME): $(OBJECTS)
	ar rcs $(RES)/$(LIBNAME) $(OBJECTS)

# $(CC) $(LDFLAGS) $(OBJECTS) $(LIBS_PATH) $(LIBS) -o $@

.cc.o:
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

clean:
	rm $(OBJECTS) $(RES)/$(LIBNAME)

