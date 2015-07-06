CC=clang++
CFLAGS=-c -Wall -Wextra -Werror -g -DSLOG_TEST_GLOBAL_INIT
SOURCES=$(wildcard *.cc)

LIBS=
DEPS=slog
DEPS_PATH=../
INCLUDE=-I./ -I../
LINKPATH=-L../
LINKLIBS=$(foreach i,$(DEPS),-l${i}) -lgtest_main -lgtest -lpthread -lboost_system -lboost_thread

OBJECTS=$(SOURCES:.cc=.o)

OUTPUT=testLog

.cc.o:
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

$(OUTPUT) : $(OBJECTS) $(foreach i,$(DEPS),$(DEPS_PATH)lib${i}.a)
	$(CC) -o $@ $^ $(GTEST_LIB) $(LINKPATH) $(LINKLIBS)
	cp ./$(OUTPUT) ./$(OUTPUT).bak
	-./$(OUTPUT)
	-rm -f ./$(OUTPUT).bak

clean:
	-rm ../*.o
	-rm *.o $(OUTPUT)
	-rm -f $(OUTPUT).bak

