#ifndef _LOG_H_
#define _LOG_H_

class LockFreeListQueue;
class Worker;
class LockFreeBuffer;

class Logger
{
    public:

        Logger(size_t size, size_t granularity);
        ~Logger();

        bool Log(const char* msg);
        bool Log(const std::string& msg);
        bool Log(const char* format,...);

    private:

        void Init();

        size_t m_sz; // total piece of buffers.
        size_t m_granularity; // size of per buffer.

        LockFreeBuffer m_buffer;
        LockFreeListQueue m_pendingMsg;
};



#include <stdio.h>

#define LOG_ENABLE (0)

#if LOG_ENABLE
    #define slog printf
#else
    #define slog(s,...)
#endif

#endif

