SRC_DIR = .
ROOT_DIR=..
RES = $(ROOT_DIR)/lib

HEADER = -I./
HEADER += -I../

LIBNAME = libmisc.a
CXX = g++
CXXFLAGS += -g -c -Wall -Wextra -mcx16

SOURCES=src/LockFreeList.cc src/LockFreeBuffer.cc src/PerThreadMemory.cc
OBJECTS = $(SOURCES:.cc=.o)

all : $(LIBNAME)

$(RES):
	mkdir -p $(RES)

.cc.o:
	$(CXX) $(CXXFLAGS) $(HEADER) $< -o $@

$(LIBNAME) : $(OBJECTS)
	ar r $(RES)/$(LIBNAME) $(OBJECTS)

clean :
	rm -f $(RES)/$(LIBNAME) $(OBJECTS)
