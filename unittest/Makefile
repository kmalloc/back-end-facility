
# Where to find user code.
USER_DIR  = .
SRC_DIR   = ../src
SRC_HEAD  = -I../include
SRC_HEAD += -I../misc

GTEST_LIB = -L$(GTEST_DIR)/lib/.libs
# Flags passed to the preprocessor.
CPPFLAGS += -I$(GTEST_DIR)/include

# Flags passed to the C++ compiler.
CXXFLAGS += -g -Wall -Wextra

TESTS = unittest 

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = -I$(GTEST_DIR)/include/gtest 
GTEST_HEADERS += -I$(GTEST_DIR)/include/gtest/internal

# House-keeping build targets.

all : $(TESTS)

thread.o : $(SRC_DIR)/thread.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/thread.cc $(SRC_HEAD)

threadtest.o : $(USER_DIR)/threadtest.cc 
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(USER_DIR)/threadtest.cc $(GTEST_HEADERS) $(SRC_HEAD)


OBJS = thread.o threadtest.o

unittest : $(OBJS) 
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(GTEST_LIB) -lpthread -lgtest_main $(OBJS) -o $(TESTS)



clean :
	rm -f $(TESTS) *.o
