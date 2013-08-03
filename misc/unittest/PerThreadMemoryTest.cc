#include "PerThreadMemory.h"
#include "ITask.h"
#include "thread.h"
#include "threadpool.h"

#include "gtest.h"

struct perThreadTestNode
{
    int a;
    int b;
    int c;
};

static const int gs_population = 32;

static void TestProc();

static PerThreadMemoryAlloc alloc(sizeof(perThreadTestNode), gs_population);
static int gs_flag = 0;

class DummyThreadForPerThreadMemoryTest:public ITask
{
    public:

        DummyThreadForPerThreadMemoryTest()
            :m_stop(false)
        {
        }

        virtual void Run()
        {
            EXPECT_EQ(0, alloc.Size());
            while (!m_stop)
            {
                TestProc();
                sched_yield();
            }
        }

        void Stop() { m_stop = true; }

        volatile bool m_stop;
};

static void SetBuff(void* buf)
{
    perThreadTestNode* node = (perThreadTestNode*)buf;
   
    node->a = 0x23;
    node->b = 0x32;
    node->c = 0xcd;
}

static void VerifyBuff(void* buf)
{
    perThreadTestNode* node = (perThreadTestNode*)buf;

    EXPECT_EQ(0x23, node->a);
    EXPECT_EQ(0x32, node->b);
    EXPECT_EQ(0xcd, node->c);
}

#include <iostream>
using namespace std;
class DummyPoolTaskForPerThreadMemTest: public ITask
{
    public:

        DummyPoolTaskForPerThreadMemTest(ThreadPool* pool, void* buff)
            :m_pool(pool), m_perThreadBuf(buff)
        {
        }

        virtual void Run()
        {
            cout << "re " << hex << m_perThreadBuf << endl;
            EXPECT_TRUE(m_perThreadBuf != NULL);
            VerifyBuff(m_perThreadBuf);

            alloc.ReleaseBuffer(m_perThreadBuf);

            sched_yield();

            void* buff = alloc.AllocBuffer();
            
            if (buff)
            {
                SetBuff(buff);
                m_pool->PostTask(new DummyPoolTaskForPerThreadMemTest(m_pool, buff));
            }
            else
            {
                cout << "run out of PerThread buff" << endl;
            }
        }

        ThreadPool* m_pool;
        void* m_perThreadBuf;
};

static void TestProc()
{
    void* buf = alloc.AllocBuffer();

    EXPECT_EQ(gs_population - 1, alloc.Size());

    alloc.ReleaseBuffer(buf);

    EXPECT_EQ(gs_population, alloc.Size());

    buf = alloc.AllocBuffer();

    memset(buf, 0x11,sizeof(perThreadTestNode));

    void* buf2 = alloc.AllocBuffer();

    memset(buf2, 0x11, sizeof(perThreadTestNode));

    EXPECT_EQ(gs_population - 2, alloc.Size());

    alloc.ReleaseBuffer(buf);
    alloc.ReleaseBuffer(buf2); 

    EXPECT_EQ(gs_population, alloc.Size());

    void* bufs[gs_population];

    for (int i = 0; i < gs_population; ++i)
    {
        bufs[i] = alloc.AllocBuffer();
        EXPECT_TRUE(bufs[i] != NULL);
        memset(bufs[i], 0x22, sizeof(perThreadTestNode));
    }
    
    EXPECT_EQ(0, alloc.Size());

    buf = alloc.AllocBuffer();
    EXPECT_EQ(NULL, buf);

    for (int i = 0; i < gs_population; ++i)
    {
        alloc.ReleaseBuffer(bufs[i]);
        EXPECT_EQ(i + 1, alloc.Size());
    }

    sched_yield();

    if (gs_flag < 2)
    {
        gs_flag++;
        alloc.FreeCurThreadMemory();
        EXPECT_EQ(0, alloc.Size());
    }
    else
    {
        EXPECT_EQ(gs_population, alloc.Size());
    }
}

TEST(PerThreadMemoryAllocTest, PTMAT)
{
    DummyThreadForPerThreadMemoryTest test_cs, test_cs2;
    Thread thread1(&test_cs);
    Thread thread2(&test_cs2);

    thread1.Start();
    thread2.Start();

    // main thread
    EXPECT_EQ(0, alloc.Size());
    TestProc();

    sleep(2);

    test_cs.Stop();
    thread1.Join(); 
    test_cs2.Stop();
    thread2.Join(); 


    //sharing buffer among threads.
    ThreadPool pool(4);

    pool.StartPooling();

    void* buff = alloc.AllocBuffer();
    EXPECT_TRUE(buff != NULL);

    SetBuff(buff);
    pool.PostTask(new DummyPoolTaskForPerThreadMemTest(&pool, buff));

    sleep(9);

    pool.StopPooling();
}

