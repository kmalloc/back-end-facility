#include "gtest.h"
#include "thread.h"
#include "ITask.h"

class DummyTask:public ITask
{
    public:
        DummyTask():counter(0) {}
        virtual ~DummyTask() {}
        virtual void Run()  { ++counter; }
        int counter;
};


TEST(operation,threadtest)
{
    using namespace std;

    DummyTask task;
    Thread thread1;
    bool detachable = false;
    thread1.SetDetachable(detachable);
    thread1.SetTask(&task);
    EXPECT_TRUE(thread1.Start());

    EXPECT_TRUE(thread1.IsRunning());


    const void* src = &task;
    const void* target = thread1.GetTask();

    EXPECT_TRUE(src == target);

    thread1.Join();

    EXPECT_EQ(1,task.counter);
    EXPECT_FALSE(thread1.IsRunning());
    EXPECT_FALSE(thread1.IsDetachable());

    EXPECT_EQ(&task,thread1.GetTask());
    EXPECT_TRUE(thread1.Start());

    EXPECT_TRUE(thread1.IsRunning());

    thread1.Join();
    EXPECT_EQ(2,task.counter);
}
