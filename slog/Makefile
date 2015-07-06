CC=clang++
CFLAGS=-c -Wall -Wextra -g -Werror -DSLOG_TEST_GLOBAL_INIT
SOURCES=$(wildcard *.cc)

LIBS=
INCLUDE=-I./

OBJECTS=$(SOURCES:.cc=.o)

OUTPUT=libslog.a

all: $(SOURCES) $(OUTPUT)

%.o:%.cc
	$(CC) $(CFLAGS) $(INCLUDE) $< -o $@

depend:.depend

.depend:$(SOURCES)
	rm -f ./.depend
	$(CC) $(CFLAGS) -MM $^>>./.depend;

include .depend

$(OUTPUT) : $(OBJECTS)
	ar rcs $@ $^

clean:
	-rm *.o $(OUTPUT) .depend

