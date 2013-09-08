CUR_DIR  = .
SRC_DIR   = ../..
SRC_HEAD  = -I../
SRC_HEAD += -I../../

CPPFLAGS  = -I$(GTEST_DIR)/include
GTEST_LIB = -L$(GTEST_DIR)/lib/.libs

XLIB_PATH = -L../../lib
XLIB    = -lxthread
XLIB    += -lmisc

CXX = g++
CXXFLAGS = -g -Wall -Wextra
TESTS = unittest

GTEST_HEADERS = -I$(GTEST_DIR)/include/gtest
GTEST_HEADERS += -I$(GTEST_DIR)/include/gtest/internal

# House-keeping build targets.

all : $(TESTS)

SpinlockQueueTest.o : $(CUR_DIR)/SpinlockQueueTest.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(CUR_DIR)/SpinlockQueueTest.cc $(GTEST_HEADERS) $(SRC_HEAD)

PerThreadMemoryTest.o : $(CUR_DIR)/PerThreadMemoryTest.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(CUR_DIR)/PerThreadMemoryTest.cc $(GTEST_HEADERS) $(SRC_HEAD) -I$(SRC_DIR)/thread

LockFreeBufferTest.o: $(CUR_DIR)/LockFreeBufferTest.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(CUR_DIR)/LockFreeBufferTest.cc $(GTEST_HEADERS) $(SRC_HEAD)  -I$(SRC_DIR)/misc

OBJS = SpinlockQueueTest.o PerThreadMemoryTest.o LockFreeBufferTest.o

unittest : $(OBJS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(GTEST_LIB) $(OBJS) $(XLIB_PATH) $(XLIB) -lpthread -lrt -lgtest_main -o $(TESTS)

clean :
	rm -f $(TESTS) *.o


