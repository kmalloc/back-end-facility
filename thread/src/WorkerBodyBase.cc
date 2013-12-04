#include "WorkerBodyBase.h"

#include <time.h>
#include <errno.h>
#include <assert.h>

class DummyExitTask: public ITask
{
    public:

        DummyExitTask(WorkerBodyBase* worker)
            :m_worker(worker)
        {
        }

        virtual ~DummyExitTask() {}

        virtual void Run()
        {
            m_worker->SetExitState();
        }

    private:

        WorkerBodyBase* m_worker;
};

/*
 *     WorkerBodyBase
 */
WorkerBodyBase::WorkerBodyBase(NotifyerBase* worker)
    :m_isRuning(false)
    ,m_timeout(5)
    ,m_reqThreshold(3)
    ,m_done(0)
    ,m_notify(false)
    ,m_notifyer(worker)
{
    sem_init(&m_sem,0,0);
}

WorkerBodyBase::~WorkerBodyBase()
{
}

void WorkerBodyBase::SignalPost()
{
    assert(0 == sem_post(&m_sem));
}

void WorkerBodyBase::SignalConsume()
{
    assert(0 == sem_wait(&m_sem));
}

bool WorkerBodyBase::SignalConsumeTimeout(int sec)
{
    int s;
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME,&ts) == -1)
        return false;

    ts.tv_sec += sec;

    while ((s = sem_timedwait(&m_sem,&ts)) == -1 && errno == EINTR)
        continue;

    return (s == 0);
}

bool WorkerBodyBase::TryConsume()
{
    return sem_trywait(&m_sem) == 0;
}

int WorkerBodyBase::Notify()
{
    if (m_notifyer)
    {
        return m_notifyer->Notify();
    }

    return 0;
}

bool WorkerBodyBase::GetRunTask(ITask*& msg)
{
    int req = 0;

    while (1)
    {
        if (!m_notify || req > m_reqThreshold)
        {
            SignalConsume();
            break;
        }
        else
        {
            if (TryConsume()) break;

            Notify();
            req++;

            if (SignalConsumeTimeout(m_timeout)) break;
        }
    }

    msg = GetInternalCmd();
    if (msg) return false;

    msg = GetTaskFromContainer();

    return true;
}

void WorkerBodyBase::Run()
{
    m_isRuning = false;
    m_ShouldStop = false;

    while (1)
    {
        ITask* msg = NULL;

        PreHandleTask();

        if (GetRunTask(msg))
        {
            m_isRuning = true;

            HandleTask(msg);

            m_isRuning = false;

            ++m_done;
        }
        else
        {
            ProcessInternalCmd(msg);
        }

        if (m_ShouldStop) break;
    }
}

void WorkerBodyBase::ClearAllTask()
{
    while (HasTask())
    {
        ITask* msg = TryGetTask();
        if (msg) delete(msg);
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
    return m_cmd.PushBack(task);
}

ITask* WorkerBodyBase::GetInternalCmd()
{
    ITask* task;
    if (!m_cmd.IsEmpty() && m_cmd.PopFront(&task)) return task;

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
    m_ShouldStop = true;
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

int WorkerBodyBase::GetTaskNumber()
{
    return GetContainerSize() + (m_isRuning?1:0);
}

bool WorkerBodyBase::IsRunning() const
{
    return m_isRuning;
}


