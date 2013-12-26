#include "ThreadPool.h"

#include "sys/Log.h"
#include "sys/AtomicOps.h"
#include "misc/SpinlockQueue.h"

#include <map>
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
        int  PickIdleWorker() const;
        virtual bool HasTask();
        virtual bool StopRunning();
        virtual int  GetContainerSize() const;

        /*
         * worker calls this function to require task to Run
         * keep in mind that this function will be called in different threads
         */
        int SetWorkerNotify(NotifyerBase* worker, int type = 0);

    protected:

        virtual void PrepareWorker();
        virtual bool HandleTask(ITask*);
        virtual ITask* GetTaskFromContainer();
        virtual bool PushTaskToContainer(ITask*);

        void DispatchTask(ITask*);
        Worker* SelectFreeWorker() const;

    private:

        ThreadPool* pool_;
        unsigned long long freeWorkerBit_;
        const int workerNum_;

        sem_t workerNotify_;
        Worker* freeWorker_;//keep this variable using in dispatch thread only, eliminating volatile
        std::vector<Worker*> workers_;
        SpinlockQueue<ITask*, PriorityQueue<ITask*,CompareTaskPriority> > queue_;
};

Dispatcher::Dispatcher(ThreadPool* pool, int workerNum)
    :WorkerBodyBase()
    ,pool_(pool)
    ,freeWorkerBit_(0)
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

    freeWorkerBit_ = (~(unsigned long long)1) << (sizeof(unsigned long long) << 3) - workerNum_;
    freeWorkerBit_ >>= (sizeof(unsigned long long) << 3) - workerNum_;

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
    {
        workers_[i]->StartWorking();
    }
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

int Dispatcher::GetContainerSize() const
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


Worker* Dispatcher::SelectFreeWorker() const
{
    /*
    for (int i = 0; i < workerNum_; ++i)
    {
        int sz = workers_[i]->GetTaskNumber();
        if (sz == 0)
        {
            return workers_[i];
        }
    }
    n = 010011
    n+1 = 010100
    ~(n+1) = 101011

    n & ~(n+1) = 000 011

    */

    unsigned long long sel = (freeWorkerBit_ & (~freeWorkerBit_ + 1));

    if (sel)
    {
        int s = 0, e = workerNum_ - 1;
        int m;

        int c = -1;
        unsigned long long i = 0;

        while (s <= e)
        {
            m = (s + e) >> 1;
            i = ((unsigned long long)1 << m);

            if (i == sel)
            {
                c = m;
                break;
            }
            else if (i < sel)
            {
                s = m + 1;
            }
            else
            {
                e = m - 1;
            }
        }

        assert(c >= 0 && c < workerNum_);
        return workers_[c];
    }

    return NULL;
}

int Dispatcher::PickIdleWorker() const
{
    int miniTask = ((unsigned int)-1) >> 1;
    for (int i = 0; miniTask && i < workerNum_; ++i)
    {
        int sz = workers_[i]->GetTaskNumber();
        if (sz < miniTask)
        {
            miniTask = sz;
        }
    }

    return miniTask;
}

void Dispatcher::PrepareWorker()
{
    if (freeWorker_) return;

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
    int affinity = task->GetAffinity();
    if (affinity < 0 || affinity >= workerNum_)
    {
        assert(freeWorker_);

        int id = freeWorker_->GetWorkerId();
        atomic_fetch_and_and(&freeWorkerBit_, ~(1 << id));

        task->SetThreadId(id);

        freeWorker_->PostTask(task);
        freeWorker_ = NULL;
    }
    else
    {
        int workid = affinity;
        Worker* worker = workers_[workid];

        if (worker == freeWorker_) freeWorker_ = NULL;

        atomic_fetch_and_and(&freeWorkerBit_, ~(1 << workid));
        task->SetThreadId(workid);

        worker->PostTask(task);
    }
}

int Dispatcher::SetWorkerNotify(NotifyerBase* worker, int type)
{
    if (worker)
    {
         if (type < 0)
         {
             Worker* work = dynamic_cast<Worker*>(worker);
             if (work == NULL) return 0;

             atomic_fetch_and_or(&freeWorkerBit_, (1 << work->GetWorkerId()));
         }
         else
         {
             sem_post(&workerNotify_);
         }
    }

    return 0;
}

bool Dispatcher::HandleTask(ITask* task)
{
    DispatchTask(task);

    // task not done yet, it is just been dispatched to worker.
    return false;
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

    if (num > (sizeof(long long) << 3)) num = (sizeof(long long) << 3);

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

int ThreadPool::SetWorkerNotify(NotifyerBase* worker, int type)
{
    return dispatcher_->SetWorkerNotify(worker, type);
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

int ThreadPool::PickIdleWorker() const
{
    return dispatcher_->PickIdleWorker();
}

