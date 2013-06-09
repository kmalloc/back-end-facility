#include "gtest.h"

#include "SpinLockQueue.h"

#include "thread.h"

class DummyTask: public ITask
{
    public:

        DummyTask(SpinLockQueue<int>& queue):m_queue(queue)
                                             :m_full(false)
                                             :m_switch(false)
        {
        }

        ~DummyTask(){}

        void switch() { m_switch = !m_switch; }

        void Run()
        {
            if (m_switch)
            {
                m_queue.PopFront();
            }
            else if (!m_full)
            {
                m_full = !m_queue.PushBack(233)
            }
        }
        
    private:

        bool m_full;
        bool m_switch;
        SpinLockQueue<int>& m_queue;
};



TEST(pg,spQueueTest)
{
    const int tsz = 16;
    SpinLockQueue queue(100);
    Thread* threads[tsz]; 
    DummyTask* tasks[tsz];


    for (int i = 0; i < tsz; i++)
    {
        DummyTask[i] = new DummyTask(queue);
        threads[i] = new Thread(task[i]);
    }


    for (int i = 0; i < tsz; i++)
    {
        //DummyTask
        //threads.
    }

}
