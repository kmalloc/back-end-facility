#include "PerThreadMemory.h"
#include "ITask.h"
#include "thread.h"

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

    EXPECT_EQ(0, alloc.Size());
    TestProc();

    sleep(2);

    test_cs.Stop();
    thread1.Join(); 
    test_cs2.Stop();
    thread2.Join(); 
}

