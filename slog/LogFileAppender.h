#ifndef SLOG_LOGFILEAPPENDER_H_
#define SLOG_LOGFILEAPPENDER_H_

#include "Logger.h"

#include <string>
#include <time.h>
#include <stdio.h>
#include <boost/thread/mutex.hpp>

namespace slog {

class LogFile: public Logger::LogAppender
{
    public:
        explicit LogFile(const std::string& file);
        virtual ~LogFile();

        void Flush(bool gracefully);
        size_t Append(const char* msg, size_t len);

    public:
        static const size_t S_DEF_BUFF_SZ = 256 * 1024;
        size_t AppendRaw(const char* msg, size_t len);

    private:
        int m_fd;
        size_t m_cur;
        const std::string m_file;
        char m_file_buff[S_DEF_BUFF_SZ];
};

class SynLogAppender: public LogFile
{
    public:
        explicit SynLogAppender(const std::string& file)
            : LogFile(file)
        {
        }

        virtual size_t Append(const char* msg, size_t len)
        {
            boost::mutex::scoped_lock lock(m_lock);
            return LogFile::Append(msg, len);
        }

        virtual void Flush(bool gracefully)
        {
            boost::mutex::scoped_lock lock(m_lock);
            LogFile::Flush(gracefully);
        }

    private:
        boost::mutex m_lock;
};

} // namespace slog

#endif  // SLOG_LOGFILEAPPENDER_H_
