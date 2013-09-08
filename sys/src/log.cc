#include "log.h"
#include <string>
#include <fstream>
#include <assert.h>

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

void Logger::Run()
{
    while (!m_stopWorker)
    {
       sem_wait(&m_sig); 

        void* buffer;

        ofstream fout;
        fout.open(m_logFile.c_str(), ofstream::out | ofstream::app);

        if (!fout.good()) continue;

        while (m_pendingMsg.Pop(&buffer))
        {
            fout << (char*)buffer;

            m_buffer.ReleaseBuffer(buffer);
        }

        fout.close();
    }
}

void Logger::StopLogging()
{
    m_stopWorker = true;
    sem_post(&m_sig);
}

size_t Logger::Log(const char* msg)
{
    // TODO
    return 0;
}

size_t Logger::Log(const std::string& msg)
{
    // TODO
    return 0;
}

size_t Log(const char* format,...)
{
    // TODO
    return 0;
}


