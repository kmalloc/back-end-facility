SRC_DIR = .
RES = ../lib

HEADER = -I./
HEADER += -I../

LIBNAME = libsysutil.a
CXX = g++
CXXFLAGS += -g -c -Wall -Wextra

SOURCE=src/Log.cc src/Logger.cc src/Daemon.cc
OBJECTS=$(SOURCE:.cc=.o)

all : $(LIBNAME)

$(OBJECTS): | $(RES)

$(RES):
	mkdir -p $(RES)

.cc.o:
	$(CXX) $(CXXFLAGS) $(HEADER) $< -o $@


$(LIBNAME) : $(OBJECTS)
	ar r $(RES)/$(LIBNAME) $(OBJECTS)

clean :
	rm -f $(RES)/$(LIBNAME) $(OBJECTS)
