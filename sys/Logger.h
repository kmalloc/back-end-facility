#ifndef _LOG_H_
#define _LOG_H_

#include <iostream>
#include <stdlib.h>
#include <string>
#include <semaphore.h>
#include <stdarg.h>

#include "thread/Thread.h"
#include "misc/LockFreeBuffer.h"
#include "misc/LockFreeList.h"

class LockFreeListQueue;
class LockFreeBuffer;

class Logger: public ThreadBase
{
    public:

        Logger(const char* file, size_t size = 2048, size_t granularity = 512);
        ~Logger();

        size_t Log(const std::string& msg);
        size_t Log(const char* format,...);
        size_t Log(const char* format, va_list args);

        void DoFlush(std::ostream& fout);
        void Flush();

        void StopLogging();

    protected:
        
        virtual void Run();
        size_t DoLog(void* buffer);

    private:

        sem_t m_sig;

        const size_t m_size; // total piece of buffers.
        const size_t m_granularity; // size of per buffer.
        const std::string m_logFile;

        LockFreeBuffer m_buffer;
        LockFreeListQueue m_pendingMsg;

        volatile bool m_stopWorker;
};

#endif

