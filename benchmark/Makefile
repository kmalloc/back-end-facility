SRC_DIR = ..
THREAD_DIR = $(SRC_DIR)/thread
MISC_DIR = $(SRC_DIR)/misc
SYS_DIR = $(SRC_DIR)/sys
OUT = output

LIBS_THREAD = lib/libxthread.a
LIBS_SYS = lib/libsysutil.a
LIBS_MISC = lib/libmisc.a


HEADER = -I./
HEADER += -I$(SRC_DIR)
HEADER += -I$(THREAD_DIR)
HEADER += -I$(SYS_DIR)
HEADER += -I$(MISC_DIR)

CXX = g++
CXXFLAGS += -g -Wall -Wextra
EXE = lockfreebenchmark

OBJECTS = $(OUT)/lockfreebenchmark.o $(OUT)/LockFreeList.o $(OUT)/PerThreadMemory.o

all : $(EXE)

$(OBJECTS): | $(OUT)

$(OUT):
	mkdir -p $(OUT)

$(OUT)/LockFreeList.o : $(SRC_DIR)/misc/src/LockFreeList.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/misc/src/LockFreeList.cc $(HEADER) -o $(OUT)/LockFreeList.o -mcx16

$(OUT)/PerThreadMemory.o : $(SRC_DIR)/misc/src/PerThreadMemory.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/misc/src/PerThreadMemory.cc $(HEADER) -o $(OUT)/PerThreadMemory.o

$(OUT)/lockfreebenchmark.o : lockfreebenchmark.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c lockfreebenchmark.cc $(HEADER) -o $(OUT)/lockfreebenchmark.o

$(EXE) : $(OBJECTS)
	$(CXX) -o $(EXE) $(OBJECTS) $(SRC_DIR)/$(LIBS_THREAD) $(SRC_DIR)/$(LIBS_SYS) $(SRC_DIR)/$(LIBS_MISC) -lpthread

clean :
	rm -f $(EXE) $(OUT)/*.o
