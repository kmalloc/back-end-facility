#include "gtest.h"

#include "SpinlockQueue.h"
#include "thread/Thread.h"
#include "thread/ITask.h"

static const int val1 = 233;
static const int val2 = 111;
static const int val3 = -23;

class slqTestDummyTask: public ITask
{
    public:

        slqTestDummyTask(SpinlockQueue<int>& queue):m_full(false)
                                             ,m_switch(false)
                                             ,m_exit(false)
                                             ,m_queue(queue)
        {
        }

        ~slqTestDummyTask(){}

        void Switch() { m_switch = !m_switch; }
        void Stop() { m_exit = true; }

        virtual void Run()
        {
            while(1)
            {
                if (m_exit) return;

                if (m_switch)
                {
                    m_queue.PopFront();
                }
                else if (!m_full)
                {
                    m_full = !m_queue.PushBack(val1);
                }

                sleep(1);
            }
        }
        
    private:

        bool m_full;
        bool m_switch;
        bool m_exit;
        SpinlockQueue<int>& m_queue;
};

void check(int& val)
{
    EXPECT_TRUE((val == val1) || (val == val2) || (val == val3));
}


void check2(SpinlockQueue<int>& queue)
{
    while (!queue.IsEmpty())
    {
        int val;
        if (!queue.PopFront(&val)) return;
        check(val);
    }
}

TEST(pg,spQueueTest)
{
    const int tsz = 16;
    const int ssz = 100;

    SpinlockQueue<int> queue(ssz);
    Thread* threads[tsz]; 
    slqTestDummyTask* tasks[tsz];

    for (int i = 0; i < tsz; i++)
    {
        tasks[i] = new slqTestDummyTask(queue);
        threads[i] = new Thread(tasks[i]);
    }

    EXPECT_TRUE(queue.PushBack(val2)); 
    EXPECT_EQ(1,queue.Size());

    EXPECT_TRUE(queue.PushBack(val2)); 
    EXPECT_EQ(2,queue.Size());

    queue.PopFront();
    EXPECT_EQ(1,queue.Size());

    EXPECT_TRUE(queue.PushBack(val2)); 
    EXPECT_EQ(2,queue.Size());

    EXPECT_FALSE(queue.IsEmpty());

    for (int i = 0; i < tsz; i++)
    {
        EXPECT_TRUE(threads[i]->Start());
    }

    sleep(4);

    //cout << "after long run, 2:" << queue.Size();
    check2(queue);

    int sz = queue.Size();

    bool suc = queue.PushBack(val2);

    EXPECT_FALSE((sz == ssz) == suc);

    for (int i = 0; i < tsz - 2; ++i)
    {
        if (i%2) tasks[i]->Switch();
    }

    sleep(2);
    EXPECT_TRUE(queue.Size() < ssz);

    sleep(10);

    //cout << "after long run:" << queue.Size();

    for (int i = 0; i < tsz; i++)
    {
        tasks[i]->Stop();
    }

    sleep(1);

    check2(queue);
    
    for (int i = 0; i < tsz; ++i)
    {
        threads[i]->Join();
        delete tasks[i];
        delete threads[i];
    }

}

