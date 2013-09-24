#include "Log.h"
#include "Logger.h"

#if LOG_ENABLE

#define LOG_FILE_ALL     "slog_all.log"
#define LOG_FILE_DEBUG   "slog_debug.log"
#define LOG_FILE_INFO    "slog_info.log"
#define LOG_FILE_WARN    "slog_warn.log"
#define LOG_FILE_ERROR   "slog_error.log"
#define LOG_FILE_FATAL   "slog_fatal.log"

static Logger gs_log_all(LOG_FILE_ALL, 3072, 512);
static Logger gs_log_debug(LOG_FILE_DEBUG, 2048, 512);
static Logger gs_log_info(LOG_FILE_INFO, 2048, 512);
static Logger gs_log_warn(LOG_FILE_WARN, 2048, 512);
static Logger gs_log_error(LOG_FILE_ERROR, 2048, 512);
static Logger gs_log_fatal(LOG_FILE_FATAL, 2048, 512);

void slog_all(const char* format, va_list arg)
{
   gs_log_all.Log(format, arg); 
}

void slog_debug(const char* format, va_list arg)
{
   gs_log_debug.Log(format, arg); 
}

void slog_info(const char* format, va_list arg)
{
   gs_log_info.Log(format, arg); 
}

void slog_warn(const char* format, va_list arg)
{
   gs_log_warn.Log(format, arg); 
}

void slog_error(const char* format, va_list arg)
{
   gs_log_error.Log(format, arg); 
}

void slog_fatal(const char* format, va_list arg)
{
   gs_log_fatal.Log(format, arg); 
}

// TODO specify log level to filter log
// the smaller the value is the the fewer log get filterred.
int slog_level()
{
    return LOG_LEVELS;
}

#endif

