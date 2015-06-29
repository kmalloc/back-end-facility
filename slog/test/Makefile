
PROG=	test_slog
SRCS= 	main.cc TestLogStream.cc TestLogger.cc TestCacheTimeStamp.cc TestAsynLogging.cc

NOMAN=


OSTYPE!= uname -s


.ifndef MAKE_PROG
all: updatelibs
.endif

CFLAGS+= -DSLOG_TEST_GLOBAL_INIT

updatelibs:
	(cd ../ && $(MAKE) clean && $(MAKE) DEBUG_FLAGS=-g for_testing=1)

.include <bsd.prog.mk>

.if ${OSTYPE} == "Linux"
LDADD+= -lrt
.endif
