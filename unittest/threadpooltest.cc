#include "gtest.h"

#include "threadpool.h"
#include "atomic_ops.h"

#include <semaphore.h>


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

            sem_post(&m_sem);
        }

        void Stop() { m_stop = true; }

        static volatile int m_exe;
        static volatile int m_done;
        volatile bool m_stop;
        volatile bool m_busy;
        static sem_t m_sem;
};


volatile int BusyTaskForThreadPoolTest::m_exe = 0;
volatile int BusyTaskForThreadPoolTest::m_done = 0;
sem_t BusyTaskForThreadPoolTest::m_sem;

class NormalTaskForThreadPoolTest:public ITask
{
    public:
        NormalTaskForThreadPoolTest()
            :ITask(TP_HIGH),m_counter(0),m_stop(false),m_busy(false) { }
        
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
            sem_post(&m_sem);
        }

        int m_counter;
        volatile bool m_stop;
        volatile bool m_busy;
        static volatile int m_exe;
        static volatile int m_done;
        static sem_t  m_sem;
};


volatile int NormalTaskForThreadPoolTest::m_exe = 0;
volatile int NormalTaskForThreadPoolTest::m_done = 0;
sem_t NormalTaskForThreadPoolTest::m_sem;

TEST(threadpooltest, alltest)
{
    const int mbsz = 12;
    const int mnsz = 12;
    const int worker = 4;

    sem_init(&BusyTaskForThreadPoolTest::m_sem, 0, 0);
    sem_init(&NormalTaskForThreadPoolTest::m_sem, 0, 0);

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

    sleep(1);

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

    EXPECT_EQ(worker > mbsz/2?mbsz/2:worker, stop);

    int newRun = stop;
    int busyLeft = worker - newRun;

    for (int i = 0; i < newRun; ++i)
        sem_wait(&BusyTaskForThreadPoolTest::m_sem);

    sleep(1);
    EXPECT_EQ(newRun, NormalTaskForThreadPoolTest::m_exe);
    EXPECT_EQ(newRun, BusyTaskForThreadPoolTest::m_done);
    EXPECT_EQ(busyLeft, BusyTaskForThreadPoolTest::m_exe);

    for (int i = 0; i < mnsz; ++i)
    {
        if (m_norm[i]->m_busy)
            m_norm[i]->m_stop = true;
    }

    for (int i = 0; i < worker; ++i)
        sem_wait(&NormalTaskForThreadPoolTest::m_sem);

    sleep(1);
    if (mnsz - worker > worker)
    {
        EXPECT_EQ(0, BusyTaskForThreadPoolTest::m_exe);
        EXPECT_EQ(worker, NormalTaskForThreadPoolTest::m_exe);
    }

    EXPECT_EQ(worker, NormalTaskForThreadPoolTest::m_done);

    while (1)
    {
        int co = 0;
        for (int i = 0; i < mnsz; ++i)
        {
            if (m_norm[i]->m_busy)
            {
                m_norm[i]->m_stop = true;
                ++co;
            }
        }

        if (co == 0) break;

        for (int i = 0; i < co; ++i)
            sem_wait(&NormalTaskForThreadPoolTest::m_sem);
    }

    EXPECT_EQ(0, NormalTaskForThreadPoolTest::m_exe);
    EXPECT_EQ(mnsz, NormalTaskForThreadPoolTest::m_done);

    sleep(2);
    if (mbsz - worker > worker)
    {
        EXPECT_EQ(worker, BusyTaskForThreadPoolTest::m_exe);
    }
    else
    {
        EXPECT_EQ(mbsz - worker, BusyTaskForThreadPoolTest::m_exe);
    }

    while (1)
    {
        int co = 0;
        for (int i = 0; i < mbsz; ++i)
        {
            if (m_busy[i]->m_busy)
            {
                m_busy[i]->m_stop = true;
                ++co;
            }
        }

        if (co == 0) break;

        for (int i = 0; i < co; ++i)
            sem_wait(&BusyTaskForThreadPoolTest::m_sem);
    }

    EXPECT_EQ(mbsz, BusyTaskForThreadPoolTest::m_done);
    EXPECT_EQ(0, NormalTaskForThreadPoolTest::m_exe);
    EXPECT_EQ(0, BusyTaskForThreadPoolTest::m_exe);

    EXPECT_EQ(0, pool.GetTaskNumber());

    EXPECT_TRUE(pool.IsRunning());

    pool.ForceShutdown();

    EXPECT_EQ(mbsz, BusyTaskForThreadPoolTest::m_done);
    EXPECT_EQ(mnsz, NormalTaskForThreadPoolTest::m_done);
    EXPECT_FALSE(pool.IsRunning()); 

}

