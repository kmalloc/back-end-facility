#ifndef __LOG_LAYER_H__
#define __LOG_LAYER_H__

#define LOG_ENABLE (1)

#if LOG_ENABLE

#include <stdarg.h>

#define LOG_FILE_ALL     "slog_all.log"
#define LOG_FILE_DEBUG   "slog_debug.log"
#define LOG_FILE_INFO    "slog_info.log"
#define LOG_FILE_WARN    "slog_warn.log"
#define LOG_FILE_ERROR   "slog_error.log"
#define LOG_FILE_FATAL   "slog_fatal.log"

enum LogLevel
{
    LOG_ALL = 0,
    LOG_DEBUG,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,

    LOG_LEVELS
};

void slog_all(const char* format, va_list va);
void slog_debug(const char* format, va_list va);
void slog_info(const char* format, va_list va);
void slog_warn(const char* format, va_list va); 
void slog_error(const char* format, va_list va);
void slog_fatal(const char* format, va_list va);

inline void slog(LogLevel level, const char* format, ...)
{
    if (level < 0 || level >= LOG_LEVELS) return;

    typedef void (* proc)(const char*, va_list);
    static const proc arr[LOG_LEVELS] =
    {
        slog_all,
        slog_debug,
        slog_info,
        slog_warn,
        slog_error,
        slog_fatal
    };

    va_list args;
    va_start(args, format);

    (arr[level])(format, args);

    va_end(args);
}

#else
    #define slog(s,...)
#endif

#endif // __LOG_LAYER_H__

