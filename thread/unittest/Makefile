
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

worker.o : $(SRC_DIR)/worker.cc 
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/worker.cc $(GTEST_HEADERS) $(SRC_HEAD)

workertest.o : $(USER_DIR)/workertest.cc 
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(USER_DIR)/workertest.cc $(GTEST_HEADERS) $(SRC_HEAD)

threadpool.o : $(SRC_DIR)/threadpool.cc 
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(SRC_DIR)/threadpool.cc $(GTEST_HEADERS) $(SRC_HEAD)

threadpooltest.o : $(USER_DIR)/threadpooltest.cc 
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $(USER_DIR)/threadpooltest.cc $(GTEST_HEADERS) $(SRC_HEAD)

OBJS = thread.o threadtest.o worker.o workertest.o threadpool.o threadpooltest.o

unittest : $(OBJS) 
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(GTEST_LIB) -lpthread -lrt -lgtest_main $(OBJS) -o $(TESTS)


clean :
	rm -f $(TESTS) *.o

