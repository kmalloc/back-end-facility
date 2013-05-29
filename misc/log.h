#ifndef _LOG_H_
#define _LOG_H_

#if LOG_ENABLE
    #define slog printf
#else
    #define slog(s,...)
#endif

#endif

