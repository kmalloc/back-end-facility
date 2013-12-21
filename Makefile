# Where to find user code.
SRC_DIR = .
THREAD_DIR = ./thread
SYS_DIR = ./sys

LIBS = lib/libxthread.a lib/libmisc.a lib/libsysutil.a

LIBNAME = libserver.a
CXX = g++
CXXFLAGS += -g -Wall -Wextra
MAKE = make

all: $(LIBNAME)

$(LIBNAME) : $(LIBS)
	$(MAKE) -C $(THREAD_DIR)
	ar rcs $(LIBNAME) $(LIBS)

clean :
	rm -f $(LIBNAME) *.o
