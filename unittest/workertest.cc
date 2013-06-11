#include "gtest.h"

#include "worker.h"


class workerTestDummyTask:public ITask
{
    public:
        workerTestDummyTask()
        {
            m_number++;
        }

        virtual void Run() { sleep(1);++m_counter; }

        static bool AllMsgDone() { return m_counter == m_number;}


        static volatile int m_counter;
        static volatile int m_number;
};

volatile int workerTestDummyTask::m_counter = 0;
volatile int workerTestDummyTask::m_number  = 0;

TEST(WorkerTaskTest,WorkerTest)
{
    using namespace std;

    Worker worker;
    workerTestDummyTask *msg;

    EXPECT_FALSE(worker.IsRunning());

    msg = new workerTestDummyTask();
    worker.PostTask(msg);

    msg = new workerTestDummyTask();
    worker.PostTask(msg);

    msg = new workerTestDummyTask();
    worker.PostTask(msg);


    EXPECT_EQ(workerTestDummyTask::m_number, worker.GetTaskNumber());

    worker.StartWorking();
    sleep(3);
    
    while(worker.GetTaskNumber() > 0) sleep(3);
   
    sleep(3);
    EXPECT_EQ(workerTestDummyTask::m_number,workerTestDummyTask::m_counter);
    EXPECT_TRUE(workerTestDummyTask::AllMsgDone());


    for (int i = 0 ; i < 100; ++i)
    {
        msg = new workerTestDummyTask();
        worker.PostTask(msg);
    }

    worker.StopRunning();

    EXPECT_FALSE(workerTestDummyTask::AllMsgDone());
    EXPECT_TRUE(worker.GetTaskNumber() > 0);

}
