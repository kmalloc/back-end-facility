#include <gtest/gtest.h>

#include "Logger.h"
#include "LogFileAppender.h"

#include <string>
#include <iostream>
#include <boost/shared_ptr.hpp>

using namespace std;
using namespace slog;

class DummyAppender: public SynLogAppender
{
    public:
        DummyAppender()
            : SynLogAppender("test_log_file_app.log"), m_cur(0)
        {
            m_buff[0] = 0;
        }

        virtual void Flush(bool gracefully) { SynLogAppender::Flush(gracefully); }

        virtual size_t Append(const char* msg, size_t len)
        {
            SynLogAppender::Append(msg, len);

            if (m_cur >= S_MAX_BUFF_SZ) return len;

            memcpy(m_buff + m_cur, msg, len);
            m_cur += len;
            return len;
        }

        static const size_t S_MAX_BUFF_SZ = 100 * 1024;
        size_t m_cur;
        char m_buff[S_MAX_BUFF_SZ];
};

const char* test_nesting_logging2()
{
    SLOG_INFO << "nest 2: " << 3.1415 << ' ' << 0xff123 << std::endl;
    return "return from nesting 2";
}

const char* test_nesting_logging1()
{
    SLOG_ERROR << "nest 1: " << 2344 << ' ' << test_nesting_logging2();
    return "return from nesting 1";
}

TEST(test_logger, test_logger_append)
{
    boost::shared_ptr<Logger::LogAppender> da(new DummyAppender);

    Logger::InitLogging(da);

    SLOG_INFO << "whatever:" << 233 << std::string("std::string format");
    SLOG_ERROR << "HAHA:" << true << 'C' << 2.33 << ' ' << test_nesting_logging1();
    SLOG_DEBUG << "Not shown:" << "really";

    std::cout << "syn logging all log:\n"
        << boost::static_pointer_cast<DummyAppender>(da)->m_buff << std::endl;
}

TEST(test_logger, test_logger_check)
{
    SLOG_CHECK(12);
    SLOG_CHECK(true);

    ASSERT_DEATH({SLOG_CHECK(0) << "doom to crash msg msg msg" << endl;}, "");
}

