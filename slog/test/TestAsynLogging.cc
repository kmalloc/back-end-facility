#include <gtest/gtest.h>

#include <time.h>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <streambuf>

#include "Logger.h"
#include "AsynLogAppender.h"

#include <pthread.h>
#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

using namespace std;
using namespace slog;

static const int g_log_cnt = 2048;
static const int g_thread_cnt = 8;
static volatile int g_check_crash = 0;
static const char* g_log_file = "asyn_logging.log";

class DummyAsynAppender: public AsynLogAppender
{
    public:
        DummyAsynAppender()
            : AsynLogAppender(g_log_file), m_cur(0)
        {
            m_buff[0] = 0;
        }

        virtual size_t Append(const char* msg, size_t len)
        {
            AsynLogAppender::Append(msg, len);
            {
                boost::mutex::scoped_lock lock(m_lock);
                if (m_cur + len < S_MAX_BUFF_SZ)
                {
                    memcpy(m_buff + m_cur, msg, len);
                    m_cur += len;
                }
            }

            return len;
        }

        size_t m_cur;
        static const size_t S_MAX_BUFF_SZ = 4 * 1024 * 1024;
        boost::mutex m_lock;
        char m_buff[S_MAX_BUFF_SZ];
};

const char* test_nesting_logging(void* arg)
{
    char* p = reinterpret_cast<char*>(arg);
    SLOG_INFO << p << "nest func: " << 3.1415 << ' ' << 0xff123;

    return "return from nesting 2";
}

struct thread_param
{
    char msg[64];
};

void* thread_func(void* arg)
{
    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 1000000;

    char* th_msg = reinterpret_cast<char*>(arg);
    for (int i = 0; i < g_log_cnt; ++i)
    {
        SLOG_ERROR << th_msg << "msg "
            << i << ':' << test_nesting_logging(arg);

        if (((i > 100 && i < 500) || (i >= 700 && i < 999))
                && i % 24)
        {
            nanosleep(&time, NULL);
        }
    }

    if (g_check_crash)
    {
        SLOG_FATAL << th_msg << th_msg << ", important log before a fatal crash, 1 of 3.";
        SLOG_FATAL << th_msg << th_msg << ", important log before a fatal crash, 2 of 3";
        SLOG_FATAL << th_msg << th_msg << ", important log before a fatal crash, 3 of 3.";

        if (g_check_crash % 2 == 0)
        {
            SLOG_CHECK(0) << th_msg << ", doom to crash, doom to crash, msg msg msg";
        }
        else
        {
            char* p = reinterpret_cast<char*>(0x233);
            *p = 3;
        }

        SLOG_FATAL << th_msg << " after fatal error.";
    }

    return NULL;
}

void LaunchTestThread()
{
    // 4 boost threads
    vector<boost::shared_ptr<boost::thread> > bv;
    bv.reserve(4);

    static thread_param args1[4] = {{{0}}};
    for (int i = 0; i < g_thread_cnt/2; ++i)
    {
        snprintf(args1[i].msg, sizeof(args1[i].msg), "boost thread %d:", i + 1);
        boost::shared_ptr<boost::thread> t(new boost::thread(thread_func, &args1[i]));
        bv.push_back(t);
    }

    // 4 pthread
    vector<pthread_t> pv;
    pv.reserve(4);

    static thread_param args2[4] = {{{0}}};
    for (int i = 0; i < g_thread_cnt/2; ++i)
    {
        pthread_t tid;

        snprintf(args2[i].msg, sizeof(args2[i].msg), "pthread %d:", i + 1);
        pthread_create(&tid, NULL, thread_func, &args2[i]);

        pv.push_back(tid);
    }

    struct timespec time;
    time.tv_sec = 0;
    time.tv_nsec = 10000000;

    SLOG_INFO << "from between main:" << 1;
    SLOG_FATAL << "from between main:" << 2;
    nanosleep(&time, NULL);

    SLOG_INFO << "from between main:" << 3;
    SLOG_FATAL << "from between main:" << 4;
    nanosleep(&time, NULL);

    for (size_t i = 0; i < bv.size(); ++i)
    {
        bv[i]->join();
        SLOG_INFO << "boost thread "<< i << " terminated";
    }

    for (size_t i = 0; i < pv.size(); ++i)
    {
        pthread_join(pv[i], NULL);
        SLOG_INFO << "pthread " << i << " terminated";
    }
}

void VerifyOutput(const char* file)
{
    std::stringstream ss;
    std::map<string, int> num2cnt;
    for (int i = 0; i < g_log_cnt; ++i)
    {
        ss.str("");
        ss << "msg " << i;
        num2cnt[ss.str()] = 0;
    }

    std::string line;
    std::ifstream fin(file);
    while (std::getline(fin, line))
    {
        size_t pos = line.find(":msg ");
        if (pos == string::npos) continue;

        ++pos;
        size_t pos2 = line.find(":", pos);
        ASSERT_FALSE(pos2 == string::npos);

        std::string s = line.substr(pos, pos2 - pos);
        std::map<string, int>::iterator it = num2cnt.find(s);
        ASSERT_FALSE(it == num2cnt.end());
        it->second++;
    }

    for (std::map<string, int>::iterator it = num2cnt.begin();
            it != num2cnt.end(); ++it)
    {
        EXPECT_EQ(it->second, g_thread_cnt) << "error:\"" << it->first << "\"";
    }
}

void CheckLog()
{
    {
        std::ofstream fin(g_log_file);
        fin << "asyn logging starting here." << std::endl;
    }

    boost::shared_ptr<Logger::LogAppender> da(new DummyAsynAppender);
    Logger::InitLogging(da, Logger::LL_DEBUG, true);

    SLOG_INFO  << "";
    SLOG_INFO  << "main:whatever:" << 233 << std::string("std::string format");
    SLOG_ERROR << "main:HAHA:" << true << 'C' << 2.33 << ' ';
    SLOG_DEBUG << "main:Not shown:" << "really";

    LaunchTestThread();

    da->Flush(true);
    sleep(1);
    VerifyOutput(g_log_file);
}

TEST(test_asyn_logging, test_log)
{
    CheckLog();
    char* env = getenv("crash_me");
    if (env)
    {
        g_check_crash = atoi(env);
        cout << "g_check_crash:" << g_check_crash << endl;
        CheckLog();
    }
}

