NOMAN=

LIB=slog
SRCS = CacheTimeStamp.cc Logger.cc LogStream.cc LogFileAppender.cc AsynLogAppender.cc
NOMAN=
INTERNALLIB=

CFLAGS+= -I${TOPDIR}
CXXFLAGS += -Werror -Wall

.ifdef for_testing
CFLAGS+= -DSLOG_TEST_GLOBAL_INIT
.endif

.include <bsd.lib.mk>

