#include "WorkerBodyBase.h"

#include <time.h>
#include <errno.h>
#include <assert.h>

class DummyExitTask: public ITask
{
    public:

        explicit DummyExitTask(WorkerBodyBase* worker)
            :worker_(worker)
        {
        }

        virtual ~DummyExitTask() {}

        virtual void Run()
        {
            worker_->SetExitState();
        }

    private:

        WorkerBodyBase* worker_;
};

/*
 *     WorkerBodyBase
 */
WorkerBodyBase::WorkerBodyBase(NotifyerBase* worker)
    :isRuning_(false)
    ,timeout_(5)
    ,reqThreshold_(3)
    ,done_(0)
    ,notify_(false)
    ,notifyer_(worker)
{
    sem_init(&sem_,0,0);
}

WorkerBodyBase::~WorkerBodyBase()
{
    sem_destroy(&sem_);
}

void WorkerBodyBase::SignalPost()
{
    assert(0 == sem_post(&sem_));
}

void WorkerBodyBase::SignalConsume()
{
    assert(0 == sem_wait(&sem_));
}

bool WorkerBodyBase::SignalConsumeTimeout(int sec)
{
    int s;
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME,&ts) == -1)
        return false;

    ts.tv_sec += sec;

    while ((s = sem_timedwait(&sem_,&ts)) == -1 && errno == EINTR)
        continue;

    return (s == 0);
}

bool WorkerBodyBase::TryConsume()
{
    return sem_trywait(&sem_) == 0;
}

int WorkerBodyBase::Notify()
{
    if (notifyer_)
    {
        return notifyer_->Notify();
    }

    return 0;
}

bool WorkerBodyBase::GetRunTask(ITask*& msg)
{
    int req = 0;

    while (1)
    {
        if (!notify_ || req > reqThreshold_)
        {
            SignalConsume();
            break;
        }
        else
        {
            if (TryConsume()) break;

            Notify();
            req++;

            if (SignalConsumeTimeout(timeout_)) break;
        }
    }

    msg = GetInternalCmd();
    if (msg) return false;

    msg = GetTaskFromContainer();

    return true;
}

int WorkerBodyBase::NotifyDone()
{
    if (!notify_ || !notifyer_) return true;

    notifyer_->Notify(-1);
}

void WorkerBodyBase::Run()
{
    isRuning_ = false;
    ShouldStop_ = false;

    while (1)
    {
        ITask* msg = NULL;

        PrepareWorker();

        if (GetRunTask(msg))
        {
            isRuning_ = true;

            bool done = HandleTask(msg);

            if (done) CleanRunTask(msg);

            ++done_;
            isRuning_ = false;

        }
        else
        {
            ProcessInternalCmd(msg);
        }

        if (ShouldStop_) break;

        NotifyDone();
    }
}

void WorkerBodyBase::CleanRunTask(ITask* task)
{
    if (task && task->ShouldDelete()) delete task;
}

void WorkerBodyBase::ClearAllTask()
{
    while (HasTask())
    {
        ITask* msg = TryGetTask();
        CleanRunTask(msg);
    }
}

ITask* WorkerBodyBase::TryGetTask()
{
    ITask* task = NULL;

    if (TryConsume())
    {
        task = GetTaskFromContainer();
    }

    return task;
}

bool WorkerBodyBase::PostTask(ITask* task)
{
    bool ret = PushTaskToContainer(task);

    if (ret) SignalPost();

    return ret;
}

bool WorkerBodyBase::PostInternalCmd(ITask* task)
{
    bool ret = PushInternalCmd(task);

    if (ret)
        SignalPost();
    else
        delete task;

    return ret;
}

bool WorkerBodyBase::PushInternalCmd(ITask* task)
{
    return cmd_.PushBack(task);
}

ITask* WorkerBodyBase::GetInternalCmd()
{
    ITask* task;
    if (!cmd_.IsEmpty() && cmd_.PopFront(&task)) return task;

    return NULL;
}

void WorkerBodyBase::ProcessInternalCmd(ITask* task)
{
    if (task == NULL) return;

    task->Run();

    delete task;
}

void WorkerBodyBase::SetExitState()
{
    ShouldStop_ = true;
}

bool WorkerBodyBase::PostExit()
{
    ITask* task;
    try
    {
         task = new DummyExitTask(this);
    }
    catch (...)
    {
        return false;
    }

    return PostInternalCmd(task);
}

bool WorkerBodyBase::StopRunning()
{
    return PostExit();
}

// note, this number is not 100% accurate.
int WorkerBodyBase::GetTaskNumber() const
{
    return GetContainerSize() + (isRuning_?1:0);
}

bool WorkerBodyBase::IsRunning() const
{
    return isRuning_;
}


