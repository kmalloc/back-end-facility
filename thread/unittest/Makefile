
# Where to find user code.
USER_DIR  = .
SRC_DIR   = ../src
SRC_HEAD  = -I../../
SRC_HEAD += -I../

GTEST_LIB = -L$(GTEST_DIR)/lib/.libs
# Flags passed to the preprocessor.

# Flags passed to the C++ compiler.
CXXFLAGS += -g -c -Wall -Wextra

XLIB_PATH = -L../../lib
XLIB  = -lxthread
XLIB += -lsysutil
XLIB += -lmisc

TESTS = unittest

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = -I$(GTEST_DIR)/include/gtest
GTEST_HEADERS += -I$(GTEST_DIR)/include/gtest/internal
GTEST_HEADERS += -I$(GTEST_DIR)/include

SOURCE=$(USER_DIR)/ThreadTest.cc $(USER_DIR)/WorkerTest.cc $(USER_DIR)/ThreadPoolTest.cc
OBJECTS=$(SOURCE:.cc=.o)

# House-keeping build targets.

all : $(TESTS)

.cc.o:
	$(CXX) $(CXXFLAGS) $(GTEST_HEADERS) $(SRC_HEAD) $< -o $@

unittest : $(OBJECTS)
	$(CXX) $(GTEST_LIB) $(OBJECTS) $(XLIB_PATH) $(XLIB) -lpthread -lrt -lgtest_main -o $(TESTS)


clean :
	rm -f $(TESTS) $(OBJECTS)

