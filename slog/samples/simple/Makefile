CC=g++
CFLAGS=-c -Wall -Wextra -g #-O2
SOURCES=main.cc

LIBS=-lslog -lboost_system -lboost_thread -lrt
LIB_PATH=-L../../
INCLUDE=-I./ -I../../
OBJECTS=$(SOURCES:.cc=.o)

EXE=run
all: $(SOURCES) $(EXE)

$(EXE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(INCLUDE) -o $@ $(LIB_PATH) $(LIBS)

.cc.o:
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

clean:
	rm *.o $(EXE)
