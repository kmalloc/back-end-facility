#include "gtest.h"

#include "worker.h"


class workerTestDummyTask:public ITask
{
    public:
        workerTestDummyTask()
        {
            m_number++;
        }

        virtual void Run() { sleep(1);m_order.push_back(m_counter);++m_counter; }
        static bool AllMsgDone() { return m_counter == m_number;}

        static int m_counter;
        static int m_number;

        static std::vector<int> m_order;
};

int workerTestDummyTask::m_counter = 0;
int workerTestDummyTask::m_number  = 0;
std::vector<int> workerTestDummyTask::m_order;

TEST(WorkerTaskTest,WorkerTest)
{

    int c = 0;
    Worker worker;
    workerTestDummyTask *msg;

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

    sleep(3);
    
    while(worker.GetTaskNumber() > 0) sleep(3);
   
    sleep(3);
    EXPECT_EQ(workerTestDummyTask::m_number,workerTestDummyTask::m_counter);
    EXPECT_TRUE(workerTestDummyTask::AllMsgDone());
    EXPECT_EQ(workerTestDummyTask::m_counter, worker.TaskDone());


    int prevTotal = workerTestDummyTask::m_number;

    EXPECT_EQ(0, worker.GetTaskNumber());

    EXPECT_EQ(prevTotal, worker.TaskDone());

    EXPECT_TRUE(worker.StopWorking(true));

    EXPECT_EQ(workerTestDummyTask::m_number,workerTestDummyTask::m_order.size());


    int ci = 0;
    for (; ci < workerTestDummyTask::m_counter; ++ci)
    {
        int ii = workerTestDummyTask::m_order[ci];
        EXPECT_EQ(ci,ii);
    }

    for (int i = 0 ; i < 100; ++i)
    {
        msg = new workerTestDummyTask();
        EXPECT_TRUE(worker.PostTask(msg)) << i;
    }

    ASSERT_TRUE(worker.StartWorking());

    sleep(1);
    EXPECT_TRUE(worker.IsRunning());

    sleep(8);

    worker.StopWorking(true);

    EXPECT_FALSE(worker.IsRunning());

    int left = worker.GetTaskNumber();
    int done = worker.TaskDone();

    EXPECT_FALSE(workerTestDummyTask::AllMsgDone());
    EXPECT_TRUE(left > 0);
    EXPECT_TRUE(done < workerTestDummyTask::m_number);

    EXPECT_TRUE(workerTestDummyTask::m_counter < workerTestDummyTask::m_number);

    EXPECT_EQ(workerTestDummyTask::m_number, done + left)
        << "total:" << workerTestDummyTask::m_number 
        << ",counter:" << workerTestDummyTask::m_counter 
        << ",done:" << done << ",left:" << left;

    for (; ci < workerTestDummyTask::m_counter; ++ci)
    {
        int ii = workerTestDummyTask::m_order[ci];
        EXPECT_EQ(ci,ii);
    }
}
