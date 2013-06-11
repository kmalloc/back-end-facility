#include "threadpool.h"
#include "SpinlockQueue.h"
#include "message.h"

struct CompareMessage 
{
    inline bool operator()(const MessageBase*& x, const MessageBase*&y) const 
    {
        return x->Priority() < y->Priority();
    }
};

class Dispatcher:public WorkerTaskBase
{
    public:

        Dispatcher(ThreadPool* pool, int workerNum = DEFAULT_WORKER_TASK_MSG_SIZE);
        ~Dispatcher();

        virtual bool PostMessage(MessageBase*);
        virtual int  GetMessageNumber();
        virtual void ClearAllMsg();

    protected:

        virtual void Run();
        virtual MessageBase* GetMessage();
        
    private:

        ThreadPool* m_pool;
        std::vector<Thread*> m_workers;
        SpinlockQueue<MessageBase*, std::priority_queue<MessageBase*, \
            std::vector<MessageBase*>, CompareMessage> > m_queue;
};


ThreadPool::ThreadPool(int num)
    :Worker(new Dispatcher(this, num))
{
}


ThreadPool::~ThreadPool()
{
}
