CUR_DIR  = .
SRC_DIR   = ../../..
SRC_HEAD  = -I../
SRC_HEAD += -I../../..

GTEST_LIB = -L$(GTEST_DIR)/lib/.libs

XLIB_PATH = -L../../../lib
XLIB    = -lxthread
XLIB    += -lsysutil
XLIB    += -lxthread
XLIB    += -lmisc
XLIB    += -lxnet

CXX = g++
CXXFLAGS = -g -c -Wall -Wextra
TESTS = unittest

GTEST_HEADERS = -I$(GTEST_DIR)/include/gtest
GTEST_HEADERS += -I$(GTEST_DIR)/include/gtest/internal
GTEST_HEADERS += -I$(GTEST_DIR)/include

SOURCE=$(CUR_DIR)/testHttpContext.cc ../HttpTask.cc ../HttpContext.cc ../HttpBuffer.cc ../HttpConnection.cc ../HttpServer.cc
OBJECTS=$(SOURCE:.cc=.o)

# House-keeping build targets.

all : $(TESTS)

.cc.o:
	$(CXX) $(CXXFLAGS) $(GTEST_HEADERS) $(SRC_HEAD) $< -o $@

unittest : $(OBJECTS)
	$(CXX) $(GTEST_LIB) $(OBJECTS) $(XLIB_PATH) $(XLIB) -lpthread -lrt -lgtest_main -o $(TESTS)

clean :
	rm -f $(TESTS) $(OBJECTS)


