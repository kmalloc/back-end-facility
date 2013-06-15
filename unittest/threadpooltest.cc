#include "gtest.h"

#include "threadpool.h"

class BusyTaskForThreadPoolTest:public ITask
{
    public:

        BusyTaskForThreadPoolTest():m_stop(false){}
        ~BusyTaskForThreadPoolTest() { ++m_dest; }

        void Run() 
        {
            m_exe++;
            while (!m_stop)
            {
                sleep(2);
            }
        }

        void Stop() { m_stop = true; }

        static int m_exe;
        static int m_dest;
        bool m_stop;
};


int BusyTaskForThreadPoolTest::m_exe = 0;
int BusyTaskForThreadPoolTest::m_dest = 0;

class NormalTaskForThreadPoolTest:public ITask
{
    public:
        NormalTaskForThreadPoolTest():m_counter(0),m_stop(false) { }
        ~NormalTaskForThreadPoolTest() { ++m_dest; }

        void Run()
        {
            m_exe++;
            while (!m_stop)
            {
                if (m_counter < 3)
                {
                    sleep(1);
                    m_counter++;
                }
                else
                {
                    break;
                }
            }
        }

        int m_counter;
        bool m_stop;
        static int m_exe;
        static int m_dest;
};


int NormalTaskForThreadPoolTest::m_dest = 0;
int NormalTaskForThreadPoolTest::m_exe = 0;


TEST(threadpooltest, alltest)
{
    const int mbsz = 12;
    const int mnsz = 12;
    const int worker = 8;
    ThreadPool pool(worker);

    BusyTaskForThreadPoolTest* m_busy[mbsz];
    NormalTaskForThreadPoolTest* m_norm[mnsz];

    
    EXPECT_FALSE(pool.IsRunning());
    EXPECT_TRUE(pool.StartPooling()); 
    EXPECT_EQ(0,pool.GetTaskNumber());


    for (int i = 0; i < mbsz; ++i)
    {
        m_busy[i] = new BusyTaskForThreadPoolTest();
        pool.PostTask(m_busy[i]);
    }

    sleep(2);

    EXPECT_TRUE(pool.IsRunning());

    EXPECT_EQ(mbsz - worker, pool.GetTaskNumber());

    for (int i = 0; i < mnsz; ++i)
    {
        m_norm[i] = new NormalTaskForThreadPoolTest();
    }

    pool.ForceShutdown();

    EXPECT_EQ(mbsz, BusyTaskForThreadPoolTest::m_dest);
    EXPECT_EQ(mnsz, NormalTaskForThreadPoolTest::m_dest);
    
}

