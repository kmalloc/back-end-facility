#ifndef _LOG_H_
#define _LOG_H_

#include <stdlib.h>
#include <string>

#include "misc/LockFreeBuffer.h"
#include "misc/lock-free-list.h"

class Worker;
class LockFreeListQueue;
class LockFreeBuffer;

class Logger
{
    public:

        Logger(size_t size, size_t granularity);
        ~Logger();

        size_t Log(const char* msg);
        size_t Log(const std::string& msg);
        size_t Log(const char* format,...);

    private:

        void Init();

        size_t m_size; // total piece of buffers.
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

