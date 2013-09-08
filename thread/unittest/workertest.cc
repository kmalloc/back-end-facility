#include "gtest.h"

#include "worker.h"

#include <semaphore.h>

class workerTestDummyTask:public ITask
{
    public:
        workerTestDummyTask():m_sleep(1)
        {
            m_number++;
        }

        virtual void Run() 
        { 
            sleep(m_sleep);
            m_order.push_back(m_counter);
            ++m_counter;
            sem_post(&m_sem);
        }

        int m_sleep;
        static bool AllMsgDone() { return m_counter == m_number;}

        static int m_counter;
        static int m_number;

        static std::vector<int> m_order;

        static sem_t m_sem;
};

int workerTestDummyTask::m_counter = 0;
int workerTestDummyTask::m_number  = 0;
std::vector<int> workerTestDummyTask::m_order;
sem_t workerTestDummyTask::m_sem;

TEST(WorkerTaskTest,WorkerTest)
{
    Worker worker;
    workerTestDummyTask *msg;

    sem_init(&workerTestDummyTask::m_sem, 0, 0);

    EXPECT_FALSE(worker.IsRunning());

    for (int i = 0; i < 4; i++)
    {
        msg = new workerTestDummyTask();
        worker.PostTask(msg);
    }

    EXPECT_EQ(workerTestDummyTask::m_number, worker.GetTaskNumber());

    ASSERT_TRUE(worker.StartWorking());
    sleep(1);
    EXPECT_TRUE(worker.IsRunning());

    for (int i = 0; i < 4; ++i)
        sem_wait(&workerTestDummyTask::m_sem);
   
    EXPECT_EQ(workerTestDummyTask::m_number,workerTestDummyTask::m_counter);
    EXPECT_TRUE(workerTestDummyTask::AllMsgDone());
    EXPECT_EQ(workerTestDummyTask::m_counter, worker.TaskDone());


    int prevTotal = workerTestDummyTask::m_number;

    EXPECT_EQ(0, worker.GetTaskNumber());

    EXPECT_EQ(prevTotal, worker.TaskDone());

    EXPECT_TRUE(worker.StopWorking(true));

    EXPECT_EQ(workerTestDummyTask::m_number, (int)workerTestDummyTask::m_order.size());


    int ci = 0;
    for (; ci < workerTestDummyTask::m_counter; ++ci)
    {
        int ii = workerTestDummyTask::m_order[ci];
        EXPECT_EQ(ci,ii);
    }

    for (int i = 0 ; i < 10; ++i)
    {
        msg = new workerTestDummyTask();
        EXPECT_TRUE(worker.PostTask(msg)) << i;
    }

    ASSERT_TRUE(worker.StartWorking());

    sleep(1);
    EXPECT_TRUE(worker.IsRunning());

    for (int i = 0; i < 10; ++i)
        sem_wait(&workerTestDummyTask::m_sem);

    worker.StopWorking(true);

    EXPECT_FALSE(worker.IsRunning());

    int left = worker.GetTaskNumber();
    int done = worker.TaskDone();

    EXPECT_TRUE(workerTestDummyTask::AllMsgDone());
    EXPECT_EQ(0, left) << "left:" << left;
    EXPECT_EQ(done, workerTestDummyTask::m_number);

    EXPECT_EQ(workerTestDummyTask::m_counter,  workerTestDummyTask::m_number);

    EXPECT_EQ(workerTestDummyTask::m_number, done + left)
        << "total:" << workerTestDummyTask::m_number 
        << ",counter:" << workerTestDummyTask::m_counter 
        << ",done:" << done << ",left:" << left;

    for (; ci < workerTestDummyTask::m_counter; ++ci)
    {
        int ii = workerTestDummyTask::m_order[ci];
        EXPECT_EQ(ci,ii);
    }

    for (int i = 0; i < 5; i++)
    {
        workerTestDummyTask * task = new workerTestDummyTask();
        task->m_sleep = 4;
        worker.PostTask(task);
    }

    EXPECT_TRUE(worker.StartWorking());

    sem_wait(&workerTestDummyTask::m_sem);
    EXPECT_TRUE(worker.GetTaskNumber() > 0);

    worker.StopWorking();

    EXPECT_TRUE(worker.GetTaskNumber() > 0);
}

