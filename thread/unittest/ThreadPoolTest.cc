#include "gtest.h"

#include "ThreadPool.h"
#include "sys/AtomicOps.h"

#include <semaphore.h>


using namespace std;

class BusyTaskForThreadPoolTest:public ITask
{
    public:

        BusyTaskForThreadPoolTest():m_busy(false) { sem_init(&m_stopSem, 0, 0); }
        ~BusyTaskForThreadPoolTest() {}

        void Run() 
        {
            m_busy = true;
            atomic_increment(&m_exe); 

            sem_wait(&m_stopSem);

            atomic_decrement(&m_exe);
            atomic_increment(&m_done);

            m_busy = false;
            sem_post(&m_sem);
        }

        static volatile int m_exe;
        static volatile int m_done;
        volatile bool m_busy;
        sem_t m_stopSem;
        static sem_t m_sem;
};


volatile int BusyTaskForThreadPoolTest::m_exe = 0;
volatile int BusyTaskForThreadPoolTest::m_done = 0;
sem_t BusyTaskForThreadPoolTest::m_sem;

class NormalTaskForThreadPoolTest:public ITask
{
    public:
        NormalTaskForThreadPoolTest()
            :ITask(TP_HIGH), m_busy(false) { sem_init(&m_stopSem, 0, 0); }
        
        ~NormalTaskForThreadPoolTest() {}

        void Run()
        {
            m_busy = true;
            atomic_increment(&m_exe);

            sem_wait(&m_stopSem);

            atomic_decrement(&m_exe);
            atomic_increment(&m_done);

            m_busy = false;
            sem_post(&m_sem);
        }

        volatile bool m_busy;
        static volatile int m_exe;
        static volatile int m_done;
        static sem_t  m_sem;
        sem_t m_stopSem;
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

    // note, busy task has lower prioprity
    BusyTaskForThreadPoolTest* m_busy[mbsz];
    NormalTaskForThreadPoolTest* m_norm[mnsz];

    EXPECT_FALSE(pool.IsRunning());
    EXPECT_TRUE(pool.StartPooling()); 
    EXPECT_EQ(0,pool.GetTaskNumber());

    // post busy task to pool
    for (int i = 0; i < mbsz; ++i)
    {
        m_busy[i] = new BusyTaskForThreadPoolTest();
        pool.PostTask(m_busy[i]);
    }

    sleep(1);

    EXPECT_TRUE(pool.IsRunning());

    EXPECT_EQ(mbsz - worker, pool.GetTaskNumber());
    EXPECT_EQ(worker, BusyTaskForThreadPoolTest::m_exe);

    // post normal task to pool
    for (int i = 0; i < mnsz; ++i)
    {
        m_norm[i] = new NormalTaskForThreadPoolTest();
        EXPECT_TRUE(pool.PostTask(m_norm[i]));
    }

    sleep(1);

    // try to stop half of the busy task
    // but since worker not equal to mbsz.
    // busy task may not all been run.
    int stop = 0;
    for (int i = 0; stop < mbsz/2 && i < mbsz; ++i)
    {
        if (m_busy[i]->m_busy)
        {
            sem_post(&m_busy[i]->m_stopSem);
            stop++;
        }
    }

    EXPECT_EQ(worker > mbsz/2?mbsz/2:worker, stop);


    cout << stop << " busy task stopped" << endl;

    int newRun = stop;
    int busyLeft = worker - newRun;

    // wati for the half stopped task to exit
    for (int i = 0; i < newRun; ++i)
        sem_wait(&BusyTaskForThreadPoolTest::m_sem);

    sleep(1);
    ASSERT_EQ(newRun, NormalTaskForThreadPoolTest::m_exe);
    EXPECT_EQ(newRun, BusyTaskForThreadPoolTest::m_done);
    EXPECT_EQ(busyLeft, BusyTaskForThreadPoolTest::m_exe);

    // try to stop all normal task
    stop = 0;
    for (int i = 0; i < mnsz; ++i)
    {
        if (m_norm[i]->m_busy)
        {
            sem_post(&m_norm[i]->m_stopSem);
            stop++;
        }
    }

    cout << stop << " normal task stopped" << endl;

    // wait for all of the normal task to exit
    for (int i = 0; i < stop; ++i)
        sem_wait(&NormalTaskForThreadPoolTest::m_sem);

    sleep(1);
    if (mnsz - worker > worker)
    {
        EXPECT_EQ(0, BusyTaskForThreadPoolTest::m_exe);
        ASSERT_EQ(worker, NormalTaskForThreadPoolTest::m_exe);
    }

    EXPECT_EQ(worker, NormalTaskForThreadPoolTest::m_done);

    while (1)
    {
        int co = 0;
        for (int i = 0; i < mnsz; ++i)
        {
            if (m_norm[i]->m_busy)
            {
                sem_post(&m_norm[i]->m_stopSem);
                ++co;
            }
        }

        if (co == 0 && NormalTaskForThreadPoolTest::m_done == mnsz) break;

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
        EXPECT_EQ(mbsz/2, BusyTaskForThreadPoolTest::m_exe);
    }

    while (1)
    {
        int co = 0;
        for (int i = 0; i < mbsz; ++i)
        {
            if (m_busy[i]->m_busy)
            {
                sem_post(&m_busy[i]->m_stopSem);
                ++co;
            }
        }

        if (co == 0 && BusyTaskForThreadPoolTest::m_done == mbsz) break;

        for (int i = 0; i < co; ++i)
            sem_wait(&BusyTaskForThreadPoolTest::m_sem);
    }

    EXPECT_EQ(mbsz, BusyTaskForThreadPoolTest::m_done);
    EXPECT_EQ(0, NormalTaskForThreadPoolTest::m_exe);
    EXPECT_EQ(0, BusyTaskForThreadPoolTest::m_exe);

    sleep(1);
    EXPECT_EQ(0, pool.GetTaskNumber());

    EXPECT_TRUE(pool.IsRunning());

    pool.StopPooling();

    EXPECT_EQ(mbsz, BusyTaskForThreadPoolTest::m_done);
    EXPECT_EQ(mnsz, NormalTaskForThreadPoolTest::m_done);
    EXPECT_FALSE(pool.IsRunning()); 

}

