#include "gtest.h"
#include "thread.h"
#include <iostream>

using namespace std;

class DummyTask:public ITask
{
    public:
        DummyTask() {}
        virtual ~DummyTask() {}
        virtual bool Run()  { cout << "dumy task running" << endl; return true;}
};


TEST(operation,threadtest)
{
    DummyTask task;
    Thread thread1;
    thread1.SetDetachable(true);
    thread1.SetTask(&task);
    thread1.Start();

    EXPECT_TRUE(thread1.IsDetachable());
}
