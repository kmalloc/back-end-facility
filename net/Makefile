CC=g++
CFLAGS=-c -Wall -g
SOURCES=src/SocketServer.cc src/SocketPoll.cc

RES=../lib
ROOT=..
LIBS_PATH=-L$(ROOT)/lib
LIBS=-lxthread -lsysutil -lmisc -lpthread -lrt

INCLUDE=-I./
INCLUDE+=-I$(ROOT)

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
	rm src/*.o

