CUR_DIR  = .
SRC_DIR   = ../..
SRC_HEAD  = -I../
SRC_HEAD += -I../../

CPPFLAGS  = -I$(GTEST_DIR)/include
GTEST_LIB = -L$(GTEST_DIR)/lib/.libs

XLIB_PATH = -L../../lib
XLIB    = -lxthread

CXX = g++
CXXFLAGS = -g -Wall -Wextra
TESTS = unittest

GTEST_HEADERS = -I$(GTEST_DIR)/include/gtest
GTEST_HEADERS += -I$(GTEST_DIR)/include/gtest/internal

# House-keeping build targets.

all : $(TESTS)

thread.o : $(SRC_DIR)/thread/src/thread.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/thread/src/thread.cc $(SRC_HEAD) -I$(SRC_DIR)/thread

SpinlockQueueTest.o : $(CUR_DIR)/SpinlockQueueTest.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(CUR_DIR)/SpinlockQueueTest.cc $(GTEST_HEADERS) $(SRC_HEAD)

OBJS = SpinlockQueueTest.o thread.o

unittest : $(OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(GTEST_LIB) $(XLIB_PATH) $(XLIB) $(OBJS) -lpthread -lrt -lgtest_main -o $(TESTS)

clean :
	rm -f $(TESTS) *.o


