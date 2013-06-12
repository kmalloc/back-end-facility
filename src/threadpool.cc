#include "threadpool.h"
#include "SpinlockQueue.h"
#include "atomic_ops.h"


struct CompareTaskPriority 
{
    inline bool operator()(const ITask*& x, const ITask*&y) const 
    {
        return x->Priority() < y->Priority();
    }
};

class Dispatcher:public WorkerBodyBase
{
    public:

        Dispatcher(ThreadPool* pool, int workerNum = DEFAULT_WORKER_TASK_MSG_SIZE);
        ~Dispatcher();

        virtual bool PostTask(ITask*);
        virtual int  GetTaskNumber();
        virtual bool HasTask() const;

        /*
         * worker calls this function to require task to Run
         * keep in mind that this function will be called in different thread
         */
        int SetWorkerNotify(WorkerBodyBase* worker);

    protected:

        virtual void HandleTask();
        virtual ITask* GetTask();
        virtual ITask* GetTaskFromContainer();
        bool HandleWorkerRequest();
        void DispatchTask(ITask*);
        Thread* SelectFreeWorker();
        
    private:

        int m_requestThreshold;
        const int m_workerNum;
        volatile int m_totalRequest;
        int m_request[m_workerNum];
        ThreadPool* m_pool;

        std::vector<Thread*> m_workers;
        SpinlockQueue<ITask*, std::priority_queue<ITask*, \
            std::vector<ITask*>, CompareTaskPriority> > m_queue;
};


Dispatcher::Dispatcher(ThreadPool* pool, int workerNum)
    :WorkerBodyBase()
    ,m_pool(pool)
    ,m_workerNum(workerNum)
    ,m_totalRequest(0)
    ,m_requestThreshold(5)
{
    m_workers.reserve(m_workerNum);
    for (int i = 0; i < m_workerNum; ++i)
    {
        Worker* worker = new Worker(m_pool, i);
        worker.EnableNotify(true);
        m_workers.push_back(worker);
        m_request[i] = 0;
    }
}


Dispatcher::~Dispatcher()
{
    for (int i = 0; i < m_workerNum; ++i)
    {
        delete m_workers[i];
    }
}


ITask* Dispatcher::PushTaskToContainer(ITask* task)
{
    return m_queue.PushBack(task);
}

ITask* Dispatcher::GetTaskFromContainer()
{
    m_queue.PopFront();
}

int Dispatcher::GetTaskNumber() 
{
    return m_queue.Size();
}

bool Dispatcher::HasTask() const 
{
    return !m_queue.IsEmpty();
}

/*
 * return one worker that is :
 * a) not busy.
 * b) with least working tasks
 *
 * considering the total amount of workers is usually rather small,
 * we don't have to apply any advanced algorithm and data structure to 
 * maintain the worker list.
 * just avoiding premptive operation and use brute-search is enough.
 */
Worker* Dispatcher::SelectFreeWorker()
{
    int chosen = -1, min_task = -1;

    for (int i = 0; i < m_workerNum; ++i)
    {
        int sz = m_workers[i]->GetTaskNumber(); 
        if (sz > min_task)
        {
            min_task = sz;
            i = chosen;
            if ( sz == 0) break;
        }
    }

    return m_workers[chosen];
}

void Dispatcher::DispatchTask(ITask* task)
{
    Worker* worker = SelectFreeWorker();
    worker->PostTask(task);
}


int Dispatcher::SetWorkerNotify(WorkerBodyBase* worker)
{
    if (m_queue.IsEmpty())
    {
        int id = worker->GetWorkerId();
        if (id < 0 || id >= m_workerNum) return -1;

        atomic_increment(&m_request[id]);
        atomic_increment(&m_totalRequest);
        PostTask(NULL);
    }
}

/*
 * in an ideal circumstance , this function should be called as least as possible.
 *
 */
bool Dispatcher::HandleWorkerRequest()
{
    if (m_totalRequest > m_requestThreshold)
    {
    }
}


void Dispatcher::HandleTask(ITask* task)
{
    if (task == NULL)
    {
        //null task serves as a special msg to require ThreadPool
        //to handle worker request.
        HandleWorkerRequest();
    }
    else
    {
        DispatchTask(task);
    }
}

/*
 * thread pool definition
 */

ThreadPool::ThreadPool(int num)
    :Worker(new Dispatcher(this, num))
{
}

ThreadPool::~ThreadPool()
{
}
