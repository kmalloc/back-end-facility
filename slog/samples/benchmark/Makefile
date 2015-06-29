CC=g++
CFLAGS=-c -Wall -Wextra -O2
SOURCES=main.cc

SLIBS=libprofiler.a libunwind.a
# -lprofiler -ltcmalloc
LIBS=-lslog -lboost_system -lboost_thread -lrt
LIB_PATH=-L../../
INCLUDE=-I./ -I../../
OBJECTS=$(SOURCES:.cc=.o)

EXE=log
all: $(SOURCES) $(EXE)

$(EXE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) $(INCLUDE) -o $@ $(LIB_PATH) $(LIBS) $(SLIBS)

.cc.o:
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

clean:
	rm *.o $(EXE)
