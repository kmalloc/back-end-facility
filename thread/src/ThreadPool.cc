#include "ThreadPool.h"

#include "sys/Log.h"
#include "sys/AtomicOps.h"
#include "misc/SpinlockQueue.h"

#include <assert.h>
#include <memory.h>
#include <unistd.h>


struct CompareTaskPriority
{
    inline bool operator()(ITask*& x, ITask*&y) const
    {
        return x->Priority() >= y->Priority();
    }
};

class Dispatcher:public WorkerBodyBase
{
    public:

        Dispatcher(ThreadPool* pool, int workerNum = DEFAULT_WORKER_TASK_MSG_SIZE);
        ~Dispatcher();

        void StartWorker();
        void StopWorker();
        void KillAllWorker();
        virtual bool HasTask();
        virtual bool StopRunning();
        virtual int  GetContainerSize();

        /*
         * worker calls this function to require task to Run
         * keep in mind that this function will be called in different threads
         */
        int SetWorkerNotify(NotifyerBase* worker);

    protected:

        virtual void PreHandleTask();
        virtual void HandleTask(ITask*);
        virtual ITask* GetTaskFromContainer();
        virtual bool PushTaskToContainer(ITask*);

        void DispatchTask(ITask*);
        Worker* SelectFreeWorker();

    private:

        ThreadPool* m_pool;
        const int m_workerNum;

        sem_t m_workerNotify;
        Worker* m_freeWorker;//keep this variable using in dispatch thread only, eliminating volatile
        std::vector<Worker*> m_workers;
        SpinlockQueue<ITask*, PriorityQueue<ITask*,CompareTaskPriority> > m_queue;
};


Dispatcher::Dispatcher(ThreadPool* pool, int workerNum)
    :WorkerBodyBase()
    ,m_pool(pool)
    ,m_workerNum(workerNum)
    ,m_freeWorker(NULL)
{
    int i;
    m_workers.reserve(m_workerNum);

    try
    {
        // just assume that push_back will not throw exception for the moment.
        // TODO
        for (i = 0; i < m_workerNum; ++i)
        {
            Worker* worker = new Worker(m_pool, i);
            worker->EnableNotify(true);
            m_workers.push_back(worker);
        }
    }
    catch (...)
    {
        for (int j = 0; j < i; ++j)
            delete m_workers[j];

        throw "out of memory";
    }

    sem_init(&m_workerNotify,0,m_workerNum);
}

Dispatcher::~Dispatcher()
{
    WorkerBodyBase::ClearAllTask();
    for (int i = 0; i < m_workerNum; ++i)
    {
        m_workers[i]->Cancel();
        delete m_workers[i];
    }

    sem_destroy(&m_workerNotify);
}

void Dispatcher::StartWorker()
{
    for (int i = 0; i < m_workerNum; ++i)
        m_workers[i]->StartWorking();
}

void Dispatcher::KillAllWorker()
{
    for (int i = 0; i < m_workerNum; ++i)
    {
        m_workers[i]->Cancel();
    }
}

void Dispatcher::StopWorker()
{
    for (int i = 0; i < m_workerNum; ++i)
    {
        m_workers[i]->StopWorking();
    }
}

bool Dispatcher::StopRunning()
{
    bool ret = WorkerBodyBase::StopRunning();

    if (ret) StopWorker();

    return ret;
}

bool Dispatcher::PushTaskToContainer(ITask* task)
{
    return m_queue.PushBack(task);
}

ITask* Dispatcher::GetTaskFromContainer()
{
    ITask* task;
    if (m_queue.PopFront(&task)) return task;

    return NULL;
}

int Dispatcher::GetContainerSize()
{
    return m_queue.Size();
}

bool Dispatcher::HasTask()
{
    return !m_queue.IsEmpty();
}

/*
 * return one worker that is free: with no task to run
 *
 * considering the total amount of workers is usually rather small,
 * we don't have to apply any advanced algorithm and data structure to
 * maintain the worker list.
 * just avoiding premptive operation and use brute-search is enough.
 */
Worker* Dispatcher::SelectFreeWorker()
{
    for (int i = 0; i < m_workerNum; ++i)
    {
        int sz = m_workers[i]->GetTaskNumber();
        if (sz == 0)
        {
            return m_workers[i];
        }
    }

    return NULL;
}

void Dispatcher::PreHandleTask()
{
    Worker* worker;

    do
    {
        sem_wait(&m_workerNotify);
        worker = SelectFreeWorker();

    } while (worker == NULL);

    m_freeWorker = worker;
}

void Dispatcher::DispatchTask(ITask* task)
{
    assert(m_freeWorker);
    m_freeWorker->PostTask(task);
}

int Dispatcher::SetWorkerNotify(NotifyerBase* worker)
{
    if (worker) return sem_post(&m_workerNotify);

    return 0;
}

void Dispatcher::HandleTask(ITask* task)
{
    DispatchTask(task);
}



/*
 * thread pool definition
 */

ThreadPool::ThreadPool(int num)
   :WorkerManagerBase()
   ,m_worker(NULL)
   ,m_dispatcher(NULL)
{
    if (num <= 0) num = CalcDefaultThreadNum();

    try
    {
        m_dispatcher = new Dispatcher(this, num);
        m_worker = new Worker(m_dispatcher);
    }
    catch (...)
    {
        if (m_worker) delete m_worker;
        if (m_dispatcher) delete m_dispatcher;

        slog(LOG_WARN, "threadpool exception, out of memory\n");
        throw "out of memory";
    }
}

ThreadPool::~ThreadPool()
{
    delete m_worker;
}

int ThreadPool::CalcDefaultThreadNum() const
{
    // glibc extention to get cpu info
    int num = sysconf(_SC_NPROCESSORS_CONF);

    slog(LOG_DEBUG, "threadpool, cpu number:%d\n", num);
    if (num <= 0) num = 2;

    return num;
}

bool ThreadPool::StartPooling()
{
    m_running = true;
    m_dispatcher->StartWorker();
    return m_worker->StartWorking();
}

bool ThreadPool::StopPooling()
{
    m_running = false;
    return m_worker->StopWorking(true);
}

void ThreadPool::ForceShutdown()
{
    m_running = false;
    m_worker->Cancel();
    m_dispatcher->KillAllWorker();
}

int ThreadPool::SetWorkerNotify(NotifyerBase* worker)
{
    return m_dispatcher->SetWorkerNotify(worker);
}

bool ThreadPool::IsRunning() const
{
    return m_running;
}

int ThreadPool::GetTaskNumber()
{
    return m_worker->GetTaskNumber();
}

bool ThreadPool::PostTask(ITask* task)
{
    return m_worker->PostTask(task);
}

