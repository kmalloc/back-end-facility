#include <gtest.h>

#include <sstream>
#include <string>

#include "thread/thread.h"
#include "log.h"

using namespace std;

static const char* gs_random_logs[] =
{
    "log log 1, number 1\n",
    "parsly, rose, sage, marry, and thyme\n",
    "If you shed tear when you miss the sun\n",
    "you also miss the stars\n",
    "Stray birds of summer come to my window to sing and fly away\n"
};

static const char* gs_log_other[] =
{
    "log log 2, number 2\n",
    "not in gs_random_logs\n",
    "whatever not in above array\n",
    "great great log log\n",
};

static const int gs_log_number = sizeof(gs_random_logs)/sizeof(sizeof(gs_random_logs[0]));
static const int gs_other_number = sizeof(gs_other_number)/sizeof(sizeof(gs_log_other[0]));

class LogThread: public ThreadBase
{
    public:

        LogThread(Logger& logger): m_logger(logger), m_stop(false) {}

        void Stop() { m_stop = true; }

        virtual void Run()
        {
            int i = 0;
            while (!m_stop)
            {
                m_logger.Log(gs_random_logs[i]);
                i = (i + 1)%gs_log_number;

                sched_yield();
                sched_yield();
            }
        }

    private:

        bool m_stop;
        Logger& m_logger;
};

TEST(LoggerTest, TestLogger)
{
    Logger logger("files.log");
    
    logger.StopLogging();
    LogThread thread1(logger), thread2(logger), thread3(logger);

    thread1.Start();
    thread2.Start();
    thread3.Start();

    sleep(5);

    thread1.Stop();
    thread2.Stop();
    thread3.Stop();

    ostringstream fout;

    logger.DoFlush(fout);

    string log = fout.str();

    for (int i = 0; i < gs_log_number; ++i)
    {
       EXPECT_TRUE(string::npos != log.find(gs_random_logs[i])); 
    }

    for (int i = 0; i < gs_other_number; ++i)
    {
       EXPECT_TRUE(string::npos == log.find(gs_log_other[i])); 
    }
}

