#include "log.h"


class LogWorkerTask: public ITask
{
};

Logger::Logger(size_t size, size_t granularity)
    :m_size(size), m_granularity(granularity)
    ,m_buffer(m_size, m_granularity)
{
}

Logger::~Logger()
{
}

void Logger::Init()
{

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


