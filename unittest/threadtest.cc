#include "gtest.h"
#include "thread.h"
#include <iostream>

using namespace std;

class DummyTask:public ITask
{
    public:
        DummyTask():counter(0) {}
        virtual ~DummyTask() {}
        virtual bool Run()  { cout << "dumy task running" << endl;++counter; return true;}
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

    thread1.Join();
    cout << " test 2" << endl;
    cout << "counter:"<<task.counter<<endl;
    EXPECT_FALSE(thread1.IsDetachable());
}
