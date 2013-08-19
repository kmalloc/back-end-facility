#ifndef _LOG_H_
#define _LOG_H_

class Logger
{
    public:

        Logger(size_t buffer);
        ~Logger();

        bool Log(const char* msg);
        bool Log(const string& msg);
        bool Log(const char* format,...);

    private:

        void Init();
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

