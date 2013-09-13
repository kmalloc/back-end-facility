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

OBJECTS = $(OUT)/Thread.o $(OUT)/Worker.o $(OUT)/ThreadPool.o

all : $(LIBNAME)

$(OBJECTS): | $(OUT)
$(OBJECTS): | $(RES)

$(OUT):
	mkdir -p $(OUT)
$(RES):
	mkdir -p $(RES)

$(OUT)/Thread.o : $(SRC_DIR)/src/Thread.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/src/Thread.cc $(HEADER) -o $(OUT)/Thread.o

$(OUT)/Worker.o : $(SRC_DIR)/src/Worker.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/src/Worker.cc $(HEADER) -o $(OUT)/Worker.o

$(OUT)/ThreadPool.o : $(SRC_DIR)/src/ThreadPool.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/src/ThreadPool.cc $(HEADER) -o $(OUT)/ThreadPool.o


$(LIBNAME) : $(OBJECTS)
	ar rcs $(RES)/$(LIBNAME) $(OBJECTS)

clean :
	rm -f $(RES)/$(LIBNAME) $(OUT)/*.o
