#include "gtest.h"
#include "thread.h"



class DummyTask:public ITask
{
    public:
        DummyTask() {}
        ~DummyTask() {}
        virtual void* run(void*)  { cout << "dumy task running" << endl;}
};


TEST(operation,threadtest)
{
    DummyTask task;
    Thread thread1();
    thread1.SetDetachable();
    thread1.SetTask(&task);
}
