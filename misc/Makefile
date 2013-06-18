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

LIBNAME = libxthread.a
CXX = g++
CXXFLAGS += -g -Wall -Wextra

OBJECTS = $(OUT)/thread.o $(OUT)/worker.o $(OUT)/threadpool.o

all : $(LIBNAME)

$(OBJECTS): | $(OUT)
$(OBJECTS): | $(RES)

$(OUT):
	mkdir -p $(OUT)
$(RES):
	mkdir -p $(RES)

$(OUT)/thread.o : $(SRC_DIR)/src/thread.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/src/thread.cc $(HEADER) -o $(OUT)/thread.o

$(OUT)/worker.o : $(SRC_DIR)/src/worker.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/src/worker.cc $(HEADER) -o $(OUT)/worker.o

$(OUT)/threadpool.o : $(SRC_DIR)/src/threadpool.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/src/threadpool.cc $(HEADER) -o $(OUT)/threadpool.o


$(LIBNAME) : $(OBJECTS)
	ar r $(RES)/$(LIBNAME) $(OBJECTS)

clean :
	rm -f $(RES)/$(LIBNAME) $(OUT)/*.o
