SRC_DIR = .
RES = ../lib

HEADER = -I.
HEADER += -I..

LIBNAME = libxthread.a
CXX = g++
CXXFLAGS += -g -c -Wall -Wextra

SOURCES=src/Thread.cc src/Worker.cc src/ThreadPool.cc src/WorkerBodyBase.cc
OBJECTS=$(SOURCES:.cc=.o)

all : $(LIBNAME)

$(RES):
	mkdir -p $(RES)

$(LIBNAME) : $(OBJECTS)
	ar rcs $(RES)/$(LIBNAME) $(OBJECTS)

.cc.o:
	$(CXX) $(CXXFLAGS) $(HEADER) $< -o $@

clean :
	rm -f $(RES)/$(LIBNAME) $(OBJECTS)
