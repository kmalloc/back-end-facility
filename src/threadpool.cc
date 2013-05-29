#include "threadpool.h"



class Dispatcher:public ITask
{
    public:
        Dispatcher(ThreadPool* pool);
        ~Dispatcher();

        virtual bool Run();
        
    private:

        ThreadPool* m_pool;
};


ThreadPool::ThreadPool(int num)
    :m_num(num)
{
    m_dispatch = new Dispatcher();      
    SetTask(m_dispatch);
}


ThreadPool::~ThreadPool()
{
   delete m_dispatch;
}
