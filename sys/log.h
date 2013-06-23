#ifndef _LOG_H_
#define _LOG_H_


#include <stdio.h>

#define LOG_ENABLE (1)

#if LOG_ENABLE
    #define slog printf
#else
    #define slog(s,...)
#endif

#endif

