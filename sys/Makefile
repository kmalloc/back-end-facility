SRC_DIR = .
OUT = output
RES = ../lib

HEADER = -I./
HEADER += -I../

LIBNAME = libsysutil.a
CXX = g++
CXXFLAGS += -g -Wall -Wextra

OBJECTS = $(OUT)/Log.o $(OUT)/Daemon.o $(OUT)/Logger.o

all : $(LIBNAME)

$(OBJECTS): | $(OUT)
$(OBJECTS): | $(RES)

$(OUT):
	mkdir -p $(OUT)
$(RES):
	mkdir -p $(RES)

$(OUT)/Log.o : $(SRC_DIR)/src/Log.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/src/Log.cc $(HEADER) -o $(OUT)/Log.o

$(OUT)/Logger.o : $(SRC_DIR)/src/Logger.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/src/Logger.cc $(HEADER) -o $(OUT)/Logger.o

$(OUT)/Daemon.o : $(SRC_DIR)/src/Daemon.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/src/Daemon.cc $(HEADER) -o $(OUT)/Daemon.o


$(LIBNAME) : $(OBJECTS)
	ar r $(RES)/$(LIBNAME) $(OBJECTS)

clean :
	rm -f $(RES)/$(LIBNAME) $(OUT)/*.o
