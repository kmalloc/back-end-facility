#include "log.h"

#include <string.h>
#include <fstream>
#include <assert.h>
#include <stdarg.h>

using namespace std;

Logger::Logger(const char* file, size_t size, size_t granularity)
    :m_size(size), m_granularity(granularity)
    ,m_buffer(m_size, m_granularity)
    ,m_pendingMsg(m_size + 1)
    ,m_logFile(file)
{
    sem_init(&m_sig, 0, 0);
    ThreadBase::Start();
}

Logger::~Logger()
{
    assert(m_stopWorker);
}

void Logger::DoFlush(ostream& fout)
{
    if (!fout.good()) return;

    void* buffer;
    while (m_pendingMsg.Pop(&buffer))
    {
        fout << (char*)buffer;
        m_buffer.ReleaseBuffer(buffer);
    }
}

void Logger::Flush()
{
    ofstream fout;
    fout.open(m_logFile.c_str(), ofstream::out | ofstream::app);
    DoFlush(fout);
    fout.close();
}

void Logger::Run()
{
    while (!m_stopWorker)
    {
        sem_wait(&m_sig); 
        Flush();
    }
}

void Logger::StopLogging()
{
    m_stopWorker = true;
    sem_post(&m_sig);
}

size_t Logger::DoLog(void* buffer)
{
    bool ret;
    do
    {
        ret = m_pendingMsg.Push(buffer);

        if (ret == false)
        {
            sem_post(&m_sig);
            sched_yield();
        }

    } while (ret == false);

    return m_granularity - 1;
}

size_t Logger::Log(const std::string& msg)
{
    size_t sz = msg.size();
    if (sz == 0) return 0;

    if (sz >= m_granularity) sz = m_granularity - 1;

    char* buffer = (char*)m_buffer.AllocBuffer();

    if (buffer == NULL) return 0;

    strncpy(buffer, msg.c_str(), sz);
    buffer[sz] = 0;

    return DoLog(buffer);
}

size_t Logger::Log(const char* format,...)
{
    char* buffer = (char*)m_buffer.AllocBuffer();
    if (buffer == NULL) return 0;

    va_list args;
    va_start(args, format);
    vsnprintf(buffer, m_granularity - 1, format, args);
    va_end(args);

    return DoLog(buffer);
}


