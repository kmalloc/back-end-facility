#include "worker.h"

#include "log.h"

#include <vector>
#include <queue>

#include <time.h>
#include <errno.h>

class DummyExitTask: public ITask
{
    public:

        DummyExitTask(){}

        virtual ~DummyExitTask() {}

        virtual void Run() { assert(0); }

};

/*
 *     WorkerBodyBase
 */

WorkerBodyBase::WorkerBodyBase(Worker* worker)
    :m_isRuning(false)
    ,m_notify(false)
    ,m_timeout(5)
    ,m_req(0)
    ,m_worker(worker)
    ,m_reqThreshold(3)
    ,m_done(0)
{
    sem_init(&m_sem,0,0);
}

WorkerBodyBase::~WorkerBodyBase()
{
}

void WorkerBodyBase::SignalPost()
{
    sem_post(&m_sem);
}

void WorkerBodyBase::SignalConsume()
{
    sem_wait(&m_sem);
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

bool WorkerBodyBase::StopRunning()
{
    return PostExit();
}

int WorkerBodyBase::Notify()
{
    if (m_worker)
    {
        return m_worker->Notify();
    }

    return 0;
}

bool WorkerBodyBase::CheckExit(ITask* task)
{
    if (task && dynamic_cast<DummyExitTask*>(task) != NULL)
    {
        delete task;
        return true;
    }

    return false;
}

void WorkerBodyBase::Run()
{
    m_isRuning = false;

    while(1)
    {
        ITask* msg = GetRunTask();

        if (CheckExit(msg)) break;

        m_isRuning = true;

        HandleTask(msg);
        
        m_isRuning = false;
        
        ++m_done;
    }
}

void WorkerBodyBase::ClearAllTask()
{
    while (HasTask())
    {
        ITask*msg = TryGetTask();
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

bool WorkerBodyBase::PostExit()
{
    ITask* task = new DummyExitTask();

    bool ret = PushTaskToContainerFront(task);

    if (ret) 
        SignalPost();
    else
        delete task;

    return ret;
}

ITask* WorkerBodyBase::GetRunTask()
{
    ITask* msg;

    while(1)
    {
        if (!m_notify || m_req > m_reqThreshold)
        {
            SignalConsume();
            break;
        }
        else
        {
            if (SignalConsumeTimeout(m_timeout))
            {
                break;
            }

            Notify();
            m_req++;
        }
    }
        
    m_req = 0;

    msg = GetTaskFromContainer();

    return msg;
}


/*
 * WorkerBody
 */
 
WorkerBody::WorkerBody(Worker*work, int maxMsgSize)
    :WorkerBodyBase(work), m_mailbox(maxMsgSize)
{
    m_mailbox.SetNullValue(NULL);
}

WorkerBody::~WorkerBody()
{
}

ITask* WorkerBody::GetTaskFromContainer()
{
    return m_mailbox.PopFront();
}

bool WorkerBody::PushTaskToContainer(ITask* task)
{
    return m_mailbox.PushBack(task);
}

bool WorkerBody::PushTaskToContainerFront(ITask* task)
{
    return m_mailbox.PushFront(task);
}

int WorkerBody::GetTaskNumber() 
{
    return m_mailbox.Size();
}

bool WorkerBody::HasTask()
{
    return !m_mailbox.IsEmpty();
}

void WorkerBody::HandleTask(ITask* task)
{
    task->Run();
    delete task;
}

/*
 *    Worker
 *
 */

Worker::Worker(WorkerManagerBase* man, int id, int maxMsgSize)
    :Thread(), m_manager(man), m_id(id)
{
   m_task = m_WorkerBody = new WorkerBody(this,maxMsgSize);
}

Worker::Worker(WorkerBodyBase* task, int id, WorkerManagerBase* man)
    :Thread(), m_manager(man), m_id(id)
{
    m_task = m_WorkerBody = task;
}

Worker::~Worker()
{
    delete m_task;
}

bool Worker::StartWorking()
{
    return Thread::Start();
}

int Worker::Notify()
{
    if (m_manager)
    {
        return m_manager->SetWorkerNotify(this); 
    }

    return 0;
}

bool Worker::StopWorking(bool join)
{
    bool ret =  m_WorkerBody->StopRunning(); 

    if (join && ret) ret = Join();

    return ret;
}

