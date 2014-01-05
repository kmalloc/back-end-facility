#ifndef _LOG_H_
#define _LOG_H_

#include <stdlib.h>
#include <string>
#include <stdarg.h>
#include <iostream>

#include "misc/NonCopyable.h"

#define LOG_MAX_PENDING (64)
#define LOG_GRANULARITY (256)
#define LOG_FLUSH_TIMEOUT (23)

#define LOG_NO_PENDING

class Logger: public noncopyable
{
    public:

        // file: specify the file to store all the log.
        Logger(const char* file);
        ~Logger();

        // in case of failure, return 0
        size_t Log(const std::string& msg);
        size_t Log(const char* format,...);
        size_t Log(const char* format, va_list args);

        // flush all the buffers in memory to disk.
        void SetOutStream(std::ostream* fout);

        static void Flush();
        static void RunLogging();
        static void StopLogging();

    private:

        std::ostream* fout_;
};

#endif

