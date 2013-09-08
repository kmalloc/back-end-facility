#include "LogLayer.h"

#include "log.h"

#if LOG_ENABLE

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

#endif

