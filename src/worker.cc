#include "worker.h"

#include <vector>
#include <queue>


/*
 *     WorkerBodyBase
 */

WorkerBodyBase::WorkerBodyBase()
    :m_isRuning(false)
    ,m_shouldStop(false)
    ,m_notifyer(NULL)
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


void WorkerBody::ClearAllTask()
{
    while (!m_mailbox.IsEmpty())
    {
        ITask*msg = m_mailbox.PopFront();
        if (msg) delete(msg);
    }
}


bool WorkerBody::PostTask(ITask* task)
{
    bool ret = m_mailbox.PushBack(task);

    if (ret) SignalPost();

    return ret;
}

int WorkerBody::GetTaskNumber() 
{
    return m_mailbox.Size();
}

ITask* WorkerBody::GetTask()
{
    ITask* msg;

    SignalConsume();
    msg = m_mailbox.PopFront();

    return msg;
}

ITask* WorkerBody::TryGetTask()
{
    ITask* task = NULL;

    if (TryConsume())
    {
        task = m_mailbox.PopFront();
    }

    return task;
}

bool WorkerBody::CheckAvailability()
{
    return m_mailbox.IsEmpty();
}

void WorkerBody::Run()
{
    m_isRuning = false;

    while(1)
    {

        if (m_shouldStop) break;

        if (CheckAvailability()) Notify();

        ITask* msg = GetTask();

        if (msg == NULL)continue;

        m_isRuning = true;
        msg->Run();

        delete msg;
        
        m_isRuning = false;
        
    }

    SetStopState(false);
}


/*
 *    Worker
 *
 */


Worker::Worker(int maxMsgSize)
    :Thread()
{
   m_task = m_WorkerBody = new WorkerBody(maxMsgSize);
}

Worker::Worker(WorkerBodyBase* task)
    :Thread()
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

