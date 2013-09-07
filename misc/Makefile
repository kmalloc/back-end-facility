SRC_DIR = .
THREAD_DIR = $(SRC_DIR)/thread
SYS_DIR = $(SRC_DIR)/sys
MISC_DIR = $(SRC_DIR)/misc
OUT = output
RES = ../lib


HEADER = -I./
HEADER += -I../
HEADER += -I$(THREAD_DIR)
HEADER += -I$(SYS_DIR)
HEADER += -I$(MISC_DIR)

LIBNAME = libmisc.a
CXX = g++
CXXFLAGS += -g -Wall -Wextra

OBJECTS = $(OUT)/lock-free-list.o $(OUT)/LockFreeBuffer.o $(OUT)/PerThreadMemory.o

all : $(LIBNAME)

$(OBJECTS): | $(OUT)
$(OBJECTS): | $(RES)

$(OUT):
	mkdir -p $(OUT)
$(RES):
	mkdir -p $(RES)

$(OUT)/lock-free-list.o : $(SRC_DIR)/src/lock-free-list.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/src/lock-free-list.cc $(HEADER) -o $(OUT)/lock-free-list.o -mcx16

$(OUT)/LockFreeBuffer.o : $(SRC_DIR)/src/LockFreeBuffer.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/src/LockFreeBuffer.cc $(HEADER) -o $(OUT)/LockFreeBuffer.o -mcx16

$(OUT)/PerThreadMemory.o : $(SRC_DIR)/src/PerThreadMemory.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/src/PerThreadMemory.cc $(HEADER) -o $(OUT)/PerThreadMemory.o -mcx16


$(LIBNAME) : $(OBJECTS)
	ar r $(RES)/$(LIBNAME) $(OBJECTS)

clean :
	rm -f $(RES)/$(LIBNAME) $(OUT)/*.o
