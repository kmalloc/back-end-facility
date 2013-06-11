#include "worker.h"
#include "message.h"

#include <vector>
#include <queue>


/*
 *     WorkerTaskBase
 */

WorkerTaskBase::WorkerTaskBase()
:m_isRuning(false), m_shouldStop(false)
{
    sem_init(&m_sem,0,0);
}

WorkerTaskBase::~WorkerTaskBase()
{
}


void WorkerTaskBase::SetStopState(bool shouldStop)
{
    m_shouldStop = shouldStop;
}

void WorkerTaskBase::SignalPost()
{
    sem_post(&m_sem);
}

void WorkerTaskBase::SignalConsume()
{
    sem_wait(&m_sem);
}

void WorkerTaskBase::StopRunning()
{
    SetStopState(true);
}



/*
 * WorkerTask
 */
 
WorkerTask::WorkerTask(int maxMsgSize)
    :WorkerTaskBase(), m_mailbox(maxMsgSize)
{
    m_mailbox.SetNullValue(NULL);
}

WorkerTask::~WorkerTask()
{
}


void WorkerTask::ClearAllMsg()
{
    while (!m_mailbox.IsEmpty())
    {
        MessageBase*msg = m_mailbox.PopFront();
        if (msg) delete(msg);
    }
}


bool WorkerTask::PostMessage(MessageBase* message)
{
    bool ret = m_mailbox.PushBack(message);

    if (ret) SignalPost();

    return ret;
}

int WorkerTask::GetMessageNumber() 
{
    return m_mailbox.Size();
}

MessageBase* WorkerTask::GetMessage()
{
    MessageBase* msg;

    SignalConsume();
    msg = m_mailbox.PopFront();

    return msg;
}

void WorkerTask::Run()
{
    while(1)
    {

        m_isRuning = false;

        if (m_shouldStop) break;

        MessageBase* msg = GetMessage();

        if (msg == NULL)continue;

        m_isRuning = true;
        msg->Run();

        delete msg;
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
   m_task = m_workerTask = new WorkerTask(maxMsgSize);
}

Worker::Worker(WorkerTaskBase* task)
    :Thread()
{
    m_task = m_workerTask = task;
}

Worker::~Worker()
{
    delete m_task;
}

bool Worker::StartWorking()
{
    return Thread::Start();
}

