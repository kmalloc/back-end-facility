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

        ThreadPool* pool_;
        const int workerNum_;

        sem_t workerNotify_;
        Worker* freeWorker_;//keep this variable using in dispatch thread only, eliminating volatile
        std::vector<Worker*> workers_;
        SpinlockQueue<ITask*, PriorityQueue<ITask*,CompareTaskPriority> > queue_;
};


Dispatcher::Dispatcher(ThreadPool* pool, int workerNum)
    :WorkerBodyBase()
    ,pool_(pool)
    ,workerNum_(workerNum)
    ,freeWorker_(NULL)
{
    int i;
    workers_.reserve(workerNum_);

    try
    {
        // just assume that push_back will not throw exception for the moment.
        // TODO
        for (i = 0; i < workerNum_; ++i)
        {
            Worker* worker = new Worker(pool_, i);
            worker->EnableNotify(true);
            workers_.push_back(worker);
        }
    }
    catch (...)
    {
        for (int j = 0; j < i; ++j)
            delete workers_[j];

        throw "out of memory";
    }

    sem_init(&workerNotify_,0,workerNum_);
}

Dispatcher::~Dispatcher()
{
    WorkerBodyBase::ClearAllTask();
    for (int i = 0; i < workerNum_; ++i)
    {
        workers_[i]->Cancel();
        delete workers_[i];
    }

    sem_destroy(&workerNotify_);
}

void Dispatcher::StartWorker()
{
    for (int i = 0; i < workerNum_; ++i)
        workers_[i]->StartWorking();
}

void Dispatcher::KillAllWorker()
{
    for (int i = 0; i < workerNum_; ++i)
    {
        workers_[i]->Cancel();
    }
}

void Dispatcher::StopWorker()
{
    for (int i = 0; i < workerNum_; ++i)
    {
        workers_[i]->StopWorking();
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
    return queue_.PushBack(task);
}

ITask* Dispatcher::GetTaskFromContainer()
{
    ITask* task;
    if (queue_.PopFront(&task)) return task;

    return NULL;
}

int Dispatcher::GetContainerSize()
{
    return queue_.Size();
}

bool Dispatcher::HasTask()
{
    return !queue_.IsEmpty();
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
    for (int i = 0; i < workerNum_; ++i)
    {
        int sz = workers_[i]->GetTaskNumber();
        if (sz == 0)
        {
            return workers_[i];
        }
    }

    return NULL;
}

void Dispatcher::PreHandleTask()
{
    Worker* worker;

    do
    {
        sem_wait(&workerNotify_);
        worker = SelectFreeWorker();

    } while (worker == NULL);

    freeWorker_ = worker;
}

void Dispatcher::DispatchTask(ITask* task)
{
    assert(freeWorker_);
    freeWorker_->PostTask(task);
}

int Dispatcher::SetWorkerNotify(NotifyerBase* worker)
{
    if (worker) return sem_post(&workerNotify_);

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
   ,worker_(NULL)
   ,dispatcher_(NULL)
{
    if (num <= 0) num = CalcDefaultThreadNum();

    try
    {
        dispatcher_ = new Dispatcher(this, num);
        worker_ = new Worker(dispatcher_);
    }
    catch (...)
    {
        if (worker_) delete worker_;
        if (dispatcher_) delete dispatcher_;

        slog(LOG_WARN, "threadpool exception, out of memory\n");
        throw "out of memory";
    }
}

ThreadPool::~ThreadPool()
{
    delete worker_;
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
    running_ = true;
    dispatcher_->StartWorker();
    return worker_->StartWorking();
}

bool ThreadPool::StopPooling()
{
    running_ = false;
    return worker_->StopWorking(true);
}

void ThreadPool::ForceShutdown()
{
    running_ = false;
    worker_->Cancel();
    dispatcher_->KillAllWorker();
}

int ThreadPool::SetWorkerNotify(NotifyerBase* worker)
{
    return dispatcher_->SetWorkerNotify(worker);
}

bool ThreadPool::IsRunning() const
{
    return running_;
}

int ThreadPool::GetTaskNumber()
{
    return worker_->GetTaskNumber();
}

bool ThreadPool::PostTask(ITask* task)
{
    return worker_->PostTask(task);
}

