CC=g++
CFLAGS=-c -Wall -g
SOURCES=main.cc HttpTask.cc HttpBuffer.cc HttpConnection.cc HttpContext.cc HttpServer.cc

ROOT=../../
LIBS_PATH=-L$(ROOT)/lib
LIBS=-lxnet -lxthread -lsysutil -lmisc -lpthread -lrt

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

