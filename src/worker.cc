#include "worker.h"

#include <vector>
#include <queue>


/*
 *     WorkerBodyBase
 */

WorkerBodyBase::WorkerBodyBase()
    :m_isRuning(false)
    ,m_shouldStop(false)
    ,m_notify(false)
{
    sem_init(&m_sem,0,0);
}

WorkerBodyBase::~WorkerBodyBase()
{
}


void WorkerBodyBase::SetStopState(bool shouldStop)
{
    m_shouldStop = shouldStop;
}

void WorkerBodyBase::SignalPost()
{
    sem_post(&m_sem);
}

void WorkerBodyBase::SignalConsume()
{
    sem_wait(&m_sem);
}

bool WorkerBodyBase::TryConsume()
{
    return sem_trywait(&m_sem) == 0;
}

void WorkerBodyBase::StopRunning()
{
    SetStopState(true);
}

int WorkerBodyBase::Notify()
{
    if (m_pool)
    {
        m_pool->SetWorkerNotify(this);
    }
}

void WorkerBodyBase::Run()
{
    m_isRuning = false;

    while(1)
    {

        if (m_shouldStop) break;

        if (m_notify && !HasTask()) Notify();

        ITask* msg = GetTask();

        m_isRuning = true;

        HandleTask(msg);
        
        m_isRuning = false;
        
    }

    SetStopState(false);
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

ITask* WorkerBodyBase::GetTask()
{
    ITask* msg;

    SignalConsume();
    msg = GetTaskFromContainer();

    return msg;
}


/*
 * WorkerBody
 */
 
WorkerBody::WorkerBody(int maxMsgSize)
    :WorkerBodyBase(), m_mailbox(maxMsgSize)
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

Worker::Worker(ThreadPool* pool, int id, int maxMsgSize)
    :Thread(), m_pool(pool), m_id(id)
{
   m_task = m_WorkerBody = new WorkerBody(maxMsgSize);
}

Worker::Worker(WorkerBodyBase* task)
    :Thread(), m_pool(NULL), m_id(-1)
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

