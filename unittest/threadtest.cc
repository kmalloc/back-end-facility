#include "gtest.h"
#include "thread.h"
#include "ITask.h"

class DummyTask:public ITask
{
    public:
        DummyTask():counter(0) {}
        virtual ~DummyTask() {}
        virtual bool Run()  { ++counter; return true;}
        int counter;
};


TEST(operation,threadtest)
{
    DummyTask task;
    Thread thread1;
    bool detachable = false;
    thread1.SetDetachable(detachable);
    thread1.SetTask(&task);
    EXPECT_TRUE(thread1.Start());

    EXPECT_TRUE(thread1.IsRunning());
    EXPECT_EQ(&task,thread1.GetTask());

    thread1.Join();

    EXPECT_EQ(1,task.counter);
    EXPECT_FALSE(thread1.IsRunning());
    EXPECT_FALSE(thread1.IsDetachable());


    EXPECT_EQ(detachable,thread1.IsDetachable());
    EXPECT_EQ(&task,thread1.GetTask());
    EXPECT_TRUE(thread1.Start());

    EXPECT_TRUE(thread1.IsRunning());

    thread1.Join();
    EXPECT_EQ(2,task.counter);
}
