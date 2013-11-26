SRC_DIR = .
ROOT_DIR=..
OUT = output
RES = $(ROOT_DIR)/lib

HEADER = -I./
HEADER += -I../

LIBNAME = libmisc.a
CXX = g++
CXXFLAGS += -g -Wall -Wextra

OBJECTS = $(OUT)/LockFreeList.o $(OUT)/LockFreeBuffer.o $(OUT)/PerThreadMemory.o

all : $(LIBNAME)

$(OBJECTS): | $(OUT)
$(OBJECTS): | $(RES)

$(OUT):
	mkdir -p $(OUT)
$(RES):
	mkdir -p $(RES)

$(OUT)/LockFreeList.o : $(SRC_DIR)/src/LockFreeList.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/src/LockFreeList.cc $(HEADER) -o $(OUT)/LockFreeList.o -mcx16

$(OUT)/LockFreeBuffer.o : $(SRC_DIR)/src/LockFreeBuffer.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/src/LockFreeBuffer.cc $(HEADER) -o $(OUT)/LockFreeBuffer.o -mcx16

$(OUT)/PerThreadMemory.o : $(SRC_DIR)/src/PerThreadMemory.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/src/PerThreadMemory.cc $(HEADER) -o $(OUT)/PerThreadMemory.o -mcx16


$(LIBNAME) : $(OBJECTS)
	ar r $(RES)/$(LIBNAME) $(OBJECTS)

clean :
	rm -f $(RES)/$(LIBNAME) $(OUT)/*.o
