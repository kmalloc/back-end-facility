#include "gtest.h"

#include "threadpool.h"
#include "atomic_ops.h"


using namespace std;

class BusyTaskForThreadPoolTest:public ITask
{
    public:

        BusyTaskForThreadPoolTest():m_stop(false), m_busy(false) {}
        ~BusyTaskForThreadPoolTest() {}

        void Run() 
        {
            m_busy = true;
            atomic_increment(&m_exe); 
            while (!m_stop)
            {
                sleep(1);
            }

            atomic_decrement(&m_exe);
            atomic_increment(&m_done);
            m_busy = false;
        }

        void Stop() { m_stop = true; }

        static volatile int m_exe;
        static volatile int m_done;
        volatile bool m_stop;
        volatile bool m_busy;
};


volatile int BusyTaskForThreadPoolTest::m_exe = 0;
volatile int BusyTaskForThreadPoolTest::m_done = 0;

class NormalTaskForThreadPoolTest:public ITask
{
    public:
        NormalTaskForThreadPoolTest():ITask(TP_HIGH),m_counter(0),m_stop(false),m_busy(false) { }
        ~NormalTaskForThreadPoolTest() {}

        void Run()
        {
            m_busy = true;
            atomic_increment(&m_exe);
            while (!m_stop)
            {
                sleep(1);
            }

            atomic_decrement(&m_exe);
            atomic_increment(&m_done);
            m_busy = false;
        }

        int m_counter;
        volatile bool m_stop;
        volatile bool m_busy;
        static volatile int m_exe;
        static volatile int m_done;
};


volatile int NormalTaskForThreadPoolTest::m_exe = 0;
volatile int NormalTaskForThreadPoolTest::m_done = 0;


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
    EXPECT_EQ(worker, BusyTaskForThreadPoolTest::m_exe);
    

    for (int i = 0; i < mnsz; ++i)
    {
        m_norm[i] = new NormalTaskForThreadPoolTest();
        EXPECT_TRUE(pool.PostTask(m_norm[i]));
    }

    sleep(1);

    int stop = 0;
    for (int i = 0; stop < mbsz/2 && i < mbsz; ++i)
    {
        if (m_busy[i]->m_busy)
        {
            m_busy[i]->m_stop = true;
            stop++;
        }
    }

    EXPECT_EQ(mbsz/2, stop);

    sleep(13);

    int newRun = mbsz/2;
    int busyLeft = worker - newRun;

    if (newRun > worker)
    {
        newRun = worker;
        busyLeft = worker - newRun;
    }

    EXPECT_EQ(newRun, NormalTaskForThreadPoolTest::m_exe);
    EXPECT_EQ(newRun, BusyTaskForThreadPoolTest::m_done);
    EXPECT_EQ(busyLeft, BusyTaskForThreadPoolTest::m_exe);

    pool.ForceShutdown();

    EXPECT_EQ(mbsz, BusyTaskForThreadPoolTest::m_done);
    EXPECT_EQ(mnsz, NormalTaskForThreadPoolTest::m_done);
    
}

