#include "Logger.h"

#include "thread/Thread.h"
#include "misc/LockFreeBuffer.h"
#include "misc/LockFreeList.h"

#include <time.h>
#include <errno.h>
#include <string.h>
#include <fstream>
#include <assert.h>
#include <iostream>
#include <pthread.h>
#include <semaphore.h>

using namespace std;

struct LogEntity
{
    string file;
    char data[0];
};

class LogWorker: public ThreadBase
{
    public:

        LogWorker(size_t max_pending_log, size_t granularity, size_t flush_time);
        ~LogWorker();

        size_t Log(const char* file, const std::string& msg);
        size_t Log(const char* file, const char* format,...);
        size_t Log(const char* file, const char* format, va_list args);

        void Flush();
        void StopLogging();

    protected:

        size_t DoLog(LogEntity* log);
        virtual void Run();

    private:

        sem_t m_sig;
        sem_t m_stop;

        const size_t m_size; // total piece of buffers cached in memory.
        const size_t m_granularity; // size of per buffer.

        LockFreeBuffer m_buffer;
        LockFreeListQueue m_pendingMsg;

        const unsigned int m_timeout;
};

LogWorker::LogWorker(size_t max_pending_log, size_t granularity, size_t flush_time)
    :m_size(max_pending_log)
    ,m_granularity(granularity)
    ,m_buffer(m_size, m_granularity + sizeof(LogEntity))
    ,m_pendingMsg(m_size + 1)
    ,m_timeout(flush_time)
{
    sem_init(&m_sig, 0, 0);
    sem_init(&m_stop,0, 0);
    ThreadBase::Start();
}

LogWorker::~LogWorker()
{
    StopLogging();
    Join();
}

void LogWorker::StopLogging()
{
    sem_post(&m_sig);
    sem_post(&m_stop);
}

void LogWorker::Flush()
{
    LogEntity* log;
    while (m_pendingMsg.Pop((void**)&log))
    {
        ofstream fout;

        fout.open(log->file.c_str(), ofstream::out | ofstream::app);
        if (!fout.good()) continue;

        fout << (char*)log->data << std::endl;

        log->~LogEntity();
        m_buffer.ReleaseBuffer(log);
    }
}

void LogWorker::Run()
{
    while (1)
    {
        int s;
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
            continue;

        ts.tv_sec += m_timeout;

        while ((s = sem_timedwait(&m_sig,&ts)) == -1 && errno == EINTR)
            continue;

        Flush();

        if (0 == sem_trywait(&m_stop)) break;
    }
}

size_t LogWorker::DoLog(LogEntity* buffer)
{
    bool ret;
    int retry = 0;

    do
    {
        ret = m_pendingMsg.Push(buffer);

        if (ret == false && retry < 16)
        {
            // msg queue is full, time to wake up worker to flush all the msg.
            ++retry;
            sem_post(&m_sig);
            sched_yield();
        }
        else if (retry >= 8)
        {
            retry = 0;
            Flush();
        }

    } while (ret == false);

    return m_granularity - 1;
}

// if lenght of msg is larger than m_granularity.
// msg will not log completely.
size_t LogWorker::Log(const char* file, const std::string& msg)
{
    size_t sz = msg.size();
    if (sz == 0) return 0;

    if (sz >= m_granularity) sz = m_granularity - 1;

    void* addr = m_buffer.AllocBuffer();
    if (addr == NULL) return 0;

    // placement new
    LogEntity* log = new(addr) LogEntity();

    log->file = file;
    char* buffer = log->data;

    strncpy(buffer, msg.c_str(), sz);
    buffer[sz] = 0;

    return DoLog(log);
}

size_t LogWorker::Log(const char* file, const char* format, va_list args)
{
    void* addr = m_buffer.AllocBuffer();
    if (addr == NULL) return 0;

    LogEntity* log = new(addr) LogEntity();

    log->file = file;
    char* buffer = log->data;

    vsnprintf(buffer, m_granularity - 1, format, args);

    return DoLog(log);
}

size_t LogWorker::Log(const char* file, const char* format,...)
{
    va_list args;
    va_start(args, format);
    size_t ret = Log(file, format, args);
    va_end(args);

    return ret;
}


static LogWorker g_worker(LOG_MAX_PENDING, LOG_GRANULARITY, LOG_FLUSH_TIMEOUT);

// definition of logger
Logger::Logger(const char* file)
    :m_logFile(file)
{
}

Logger::~Logger()
{
}

void Logger::Flush()
{
    g_worker.Flush();
}

size_t Logger::Log(const char* format, va_list args)
{
    return g_worker.Log(m_logFile.c_str(), format, args);
}

size_t Logger::Log(const string& msg)
{
    return g_worker.Log(m_logFile.c_str(), msg);
}

size_t Logger::Log(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    size_t ret = Log(format, args);
    va_end(args);

    return ret;
}

void Logger::StopLogging()
{
    g_worker.StopLogging();
}

