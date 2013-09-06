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

LIBNAME = libsysutil.a
CXX = g++
CXXFLAGS += -g -Wall -Wextra

OBJECTS = $(OUT)/log.o

all : $(LIBNAME)

$(OBJECTS): | $(OUT)
$(OBJECTS): | $(RES)

$(OUT):
	mkdir -p $(OUT)
$(RES):
	mkdir -p $(RES)

$(OUT)/log.o : $(SRC_DIR)/src/log.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/src/log.cc $(HEADER) -o $(OUT)/log.o

$(LIBNAME) : $(OBJECTS)
	ar r $(RES)/$(LIBNAME) $(OBJECTS)

clean :
	rm -f $(RES)/$(LIBNAME) $(OUT)/*.o
