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
    ostream* fout;
    char data[0];
};

class LogWorker: public ThreadBase
{
    public:

        LogWorker(size_t max_pending_log, size_t granularity, size_t flush_time);
        ~LogWorker();

        size_t Log(ostream* fout, const std::string& msg);
        size_t Log(ostream* fout, const char* format,...);
        size_t Log(ostream* fout, const char* format, va_list args);

        void Flush();
        void StopLogging();

        void RunWorker();

    protected:

        virtual void Run();
        size_t DoLog(LogEntity* log);

    private:

        sem_t sig_;
        sem_t stop_;
        pthread_mutex_t guardRunning_;

        const size_t size_; // total piece of buffers cached in memory.
        const size_t granularity_; // size of per buffer.

        // buffer Pool
        LockFreeBuffer buffer_;
        LockFreeListQueue pendingMsg_;

        const unsigned int timeout_;
};

LogWorker::LogWorker(size_t max_pending_log, size_t granularity, size_t flush_time)
    :size_(max_pending_log)
    ,granularity_(granularity)
    ,buffer_(size_, granularity_ + sizeof(LogEntity))
    ,pendingMsg_(size_ + 1)
    ,timeout_(flush_time)
{
    sem_init(&sig_, 0, 0);
    sem_init(&stop_,0, 0);
    pthread_mutex_init(&guardRunning_, NULL);
    ThreadBase::Start();
}

LogWorker::~LogWorker()
{
    StopLogging();
    Join();

    sem_destroy(&sig_);
    sem_destroy(&stop_);
    pthread_mutex_destroy(&guardRunning_);
}

void LogWorker::StopLogging()
{
    sem_post(&sig_);
    sem_post(&stop_);
}

void LogWorker::RunWorker()
{
    pthread_mutex_lock(&guardRunning_);
    if (ThreadBase::IsRunning() == false)
    {
        ThreadBase::Start();
    }
    pthread_mutex_unlock(&guardRunning_);
}

void LogWorker::Flush()
{
    LogEntity* log;
    while (pendingMsg_.Pop((void**)&log))
    {
        ostream* fout = log->fout;
        if (!fout->good()) continue;

        *fout << (char*)log->data << std::endl;

        // log->~LogEntity();
        buffer_.ReleaseBuffer(log);
    }
}

void LogWorker::Run()
{
    while (1)
    {
        struct timespec ts;
        if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
            continue;

        ts.tv_sec += timeout_;

        while (sem_timedwait(&sig_,&ts) == -1 && errno == EINTR)
            continue;

        Flush();

        if (0 == sem_trywait(&stop_)) break;
    }
}

size_t LogWorker::DoLog(LogEntity* buffer)
{
    bool ret;
    int retry = 0;

    do
    {
        ret = pendingMsg_.Push(buffer);

        if (ret == false && retry < 32)
        {
            // msg queue is full, time to wake up worker to flush all the msg.
            ++retry;
            sem_post(&sig_);
            sched_yield();
        }
        else if (retry >= 32)
        {
            // logging thread might already been stop.
            return 0;
        }

    } while (ret == false);

    return granularity_ - 1;
}

// if lenght of msg is larger than granularity_.
// msg will not log completely.
size_t LogWorker::Log(ostream* fout, const std::string& msg)
{
    size_t sz = msg.size();
    if (sz == 0) return 0;

    if (sz >= granularity_) sz = granularity_ - 1;

    void* addr = buffer_.AllocBuffer();
    if (addr == NULL) return 0;

    // placement new
    // LogEntity* log = new(addr) LogEntity();
    LogEntity* log = (LogEntity*)addr;

    log->fout = fout;
    char* buffer = log->data;

    strncpy(buffer, msg.c_str(), sz);
    buffer[sz] = 0;

    return DoLog(log);
}

size_t LogWorker::Log(ostream* fout, const char* format, va_list args)
{
    void* addr = buffer_.AllocBuffer();
    if (addr == NULL) return 0;

    // LogEntity* log = new(addr) LogEntity();
    LogEntity* log = (LogEntity*)addr;

    log->fout = fout;
    char* buffer = log->data;

    vsnprintf(buffer, granularity_ - 1, format, args);

    return DoLog(log);
}

size_t LogWorker::Log(ostream* fout, const char* format,...)
{
    va_list args;
    va_start(args, format);
    size_t ret = Log(fout, format, args);
    va_end(args);

    return ret;
}


// one worker serves all logger.
static LogWorker g_worker(LOG_MAX_PENDING, LOG_GRANULARITY, LOG_FLUSH_TIMEOUT);

// definition of logger
Logger::Logger(const char* file)
    :fout_(NULL), logfile_(file)
{
    ofstream* fout = new ofstream();
    fout->open(file, ofstream::out | ofstream::app);

    if (!fout->good()) throw "can not open file";

    fout_ = fout;
}

Logger::~Logger()
{
    if (fout_) delete fout_;
}

void Logger::Flush()
{
    g_worker.Flush();
}

size_t Logger::Log(const char* format, va_list args)
{
    return g_worker.Log(fout_, format, args);
}

size_t Logger::Log(const string& msg)
{
    return g_worker.Log(fout_, msg);
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

void Logger::RunLogging()
{
    g_worker.RunWorker();
}

void Logger::SetOutStream(ostream* fout)
{
    if (fout_) delete fout_;

    fout_ = fout;
}

