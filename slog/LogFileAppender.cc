#include "LogFileAppender.h"

#include "Likely.h"

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <assert.h>
#include <unistd.h>

namespace slog {

LogFile::LogFile(const std::string& file)
    : m_fd(open(file.c_str(), O_WRONLY | O_APPEND | O_CREAT, S_IRWXU)), m_cur(0), m_file(file)
{
    assert(m_fd != -1);
    m_file_buff[0] = 0;
}

LogFile::~LogFile()
{
    LogFile::Flush(true);
    close(m_fd);
}

size_t LogFile::AppendRaw(const char* msg, size_t len)
{
    size_t ret = 0;
    while (len)
    {
        int written = write(m_fd, msg, len);
        if (written == -1 && errno != EINTR) break;

        if (UNLIKELY(written == -1)) continue;

        ret += static_cast<size_t>(written);
        msg += written;
        len -= static_cast<size_t>(written);
    }

    return ret;
}

size_t LogFile::Append(const char* msg, size_t len)
{
    assert(len <= S_DEF_BUFF_SZ);
    if (UNLIKELY(m_cur > S_DEF_BUFF_SZ - len)) LogFile::Flush(true);

    memcpy(m_file_buff + m_cur, msg, len);
    m_cur += len;
    return len;
}

void LogFile::Flush(bool gracefully)
{
    AppendRaw(m_file_buff, m_cur);
    m_cur = 0;
    (void)gracefully;
}

}

