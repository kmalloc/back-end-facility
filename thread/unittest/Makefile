
# Where to find user code.
USER_DIR  = .
SRC_DIR   = ../src
SRC_HEAD  = -I../../
SRC_HEAD += -I../

GTEST_LIB = -L$(GTEST_DIR)/lib/.libs
# Flags passed to the preprocessor.
CPPFLAGS += -I$(GTEST_DIR)/include

# Flags passed to the C++ compiler.
CXXFLAGS += -g -Wall -Wextra

XLIB_PATH = -L../../lib
XLIB  = -lxthread
XLIB += -lsysutil
XLIB += -lxthread
XLIB += -lmisc

TESTS = unittest 

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = -I$(GTEST_DIR)/include/gtest 
GTEST_HEADERS += -I$(GTEST_DIR)/include/gtest/internal

# House-keeping build targets.

all : $(TESTS)

ThreadTest.o : $(USER_DIR)/ThreadTest.cc 
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(USER_DIR)/ThreadTest.cc $(GTEST_HEADERS) $(SRC_HEAD)

WorkerTest.o : $(USER_DIR)/WorkerTest.cc 
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(USER_DIR)/WorkerTest.cc $(GTEST_HEADERS) $(SRC_HEAD)

ThreadPoolTest.o : $(USER_DIR)/ThreadPoolTest.cc 
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(USER_DIR)/ThreadPoolTest.cc $(GTEST_HEADERS) $(SRC_HEAD)

OBJS = ThreadTest.o WorkerTest.o ThreadPoolTest.o

unittest : $(OBJS) 
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(GTEST_LIB) $(OBJS) $(XLIB_PATH) $(XLIB) -lpthread -lrt -lgtest_main -o $(TESTS)


clean :
	rm -f $(TESTS) *.o

