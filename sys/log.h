#ifndef _LOG_H_
#define _LOG_H_

#include <iostream>
#include <stdlib.h>
#include <string>
#include <semaphore.h>

#include "thread/thread.h"
#include "misc/LockFreeBuffer.h"
#include "misc/lock-free-list.h"

class LockFreeListQueue;
class LockFreeBuffer;

class Logger: public ThreadBase
{
    public:

        Logger(const char* file, size_t size = 2048, size_t granularity = 512);
        ~Logger();

        size_t Log(const std::string& msg);
        size_t Log(const char* format,...);

        void DoFlush(std::ostream& fout);
        void Flush();

        void StopLogging();

    protected:
        
        virtual void Run();
        size_t DoLog(void* buffer);

    private:

        volatile bool m_stopWorker;

        std::string m_logFile;
        size_t m_size; // total piece of buffers.
        size_t m_granularity; // size of per buffer.

        LockFreeBuffer m_buffer;
        LockFreeListQueue m_pendingMsg;

        sem_t m_sig;
};



#include <stdio.h>

#define LOG_ENABLE (0)

#if LOG_ENABLE
    #define slog printf
#else
    #define slog(s,...)
#endif

#endif

