#ifndef _LOG_H_
#define _LOG_H_

class Logger
{
    public:

        Logger(size_t buffer);
        ~Logger();

        Log(const char* msg);
        Log(const string& msg);
        Log(const char* format,...);

    private:

        char* m_buff;
};



#include <stdio.h>

#define LOG_ENABLE (0)

#if LOG_ENABLE
    #define slog printf
#else
    #define slog(s,...)
#endif

#endif

