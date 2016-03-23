#include "gtest/gtest.h"
#include "Thread.h"
#include "ITask.h"

#include <pthread.h>
#include <semaphore.h>

class ThreadTestDummyTask:public ITask
{
    public:
        ThreadTestDummyTask():m_stop(false),counter(0) { sem_init(&m_sem, 0, 0); }
        virtual ~ThreadTestDummyTask() { sem_destroy(&m_sem); }
        virtual void Run()  
        {
            sem_post(&m_sem);
            while(!m_stop)
            {
                ++counter; 
                sleep(1);
            }
        }
        void init() { m_stop = false;}
        void stop() { m_stop = true;}
        bool m_stop;
        int counter;

        sem_t m_sem;
};


TEST(operation,threadtest)
{
    using namespace std;

    ThreadTestDummyTask task;
    Thread thread1(&task);
    bool detachable = false;
    thread1.SetDetachable(detachable);
    EXPECT_TRUE(thread1.Start());

    sem_wait(&task.m_sem);
    EXPECT_TRUE(thread1.IsRunning());


    const void* src = &task;
    const void* target = thread1.GetTask();

    EXPECT_TRUE(src == target);

    sleep(3);

    task.stop();
    thread1.Join();

    EXPECT_TRUE(task.counter > 1);
    EXPECT_FALSE(thread1.IsRunning());
    EXPECT_FALSE(thread1.IsDetachable());
    EXPECT_EQ(&task, thread1.GetTask());

    task.init();
    EXPECT_TRUE(thread1.Start());

    sleep(3);
    EXPECT_TRUE(thread1.IsRunning());
    EXPECT_FALSE(thread1.IsDetachable());
    EXPECT_EQ(&task,thread1.GetTask());
    task.stop();
    thread1.Join();
    EXPECT_TRUE(task.counter > 2);


    task.init();
    EXPECT_TRUE(thread1.Start());

    sleep(2);

    EXPECT_TRUE(thread1.Cancel());
    sleep(1);

    void* ret;
    EXPECT_TRUE(thread1.Join(&ret));

    EXPECT_EQ(PTHREAD_CANCELED, ret);
}

