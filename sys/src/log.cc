#include "log.h"

Logger::Logger(const char* file, size_t size, size_t granularity)
    :m_size(size), m_granularity(granularity)
    ,m_buffer(m_size, m_granularity)
    ,m_pendingMsg(m_size + 1)
    ,m_logFile(file)
{
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
        
    }
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


