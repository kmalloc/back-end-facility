CUR_DIR  = .
SRC_DIR   = ../..
SRC_HEAD  = -I../
SRC_HEAD += -I../../

CPPFLAGS  = -I$(GTEST_DIR)/include
GTEST_LIB = -L$(GTEST_DIR)/lib/.libs

XLIB_PATH = -L../../lib
XLIB    = -lxthread
XLIB    += -lsysutil
XLIB    += -lmisc

CXX = g++
CXXFLAGS = -g -Wall -Wextra
TESTS = unittest

GTEST_HEADERS = -I$(GTEST_DIR)/include/gtest
GTEST_HEADERS += -I$(GTEST_DIR)/include/gtest/internal

# House-keeping build targets.

all : $(TESTS)

LoggerTest.o : $(CUR_DIR)/LoggerTest.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(CUR_DIR)/LoggerTest.cc $(GTEST_HEADERS) $(SRC_HEAD)

SocketPollTest.o : $(CUR_DIR)/SocketPollTest.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(CUR_DIR)/SocketPollTest.cc $(GTEST_HEADERS) $(SRC_HEAD)

OBJS = LoggerTest.o SocketPollTest.o

unittest : $(OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(GTEST_LIB) $(OBJS) $(XLIB_PATH) $(XLIB) -lpthread -lrt -lgtest_main -o $(TESTS)

clean :
	rm -f $(TESTS) *.o


