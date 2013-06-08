#include "worker.h"
#include "message.h"

#include <vector>
#include <queue>


using namespace std;

/*
 *     WorkerTask
 */

WorkerTask::WorkerTask(int msgSize)
:m_mailbox(msgSize), m_isRuning(false), m_shouldStop(false)
{
    pthread_spin_init(&m_stoplock,0);
    sem_init(&m_sem,0,0);
}



WorkerTask::~WorkerTask()
{
    while (!m_mailbox.IsEmpty())
    {
        MessageBase*msg = m_mailbox.PopFront();
        delete(msg);
    }
}



bool WorkerTask::PostMessage(MessageBase* message)
{

    bool ret = m_mailbox.PushBack(message);

    if (ret) sem_post(&m_sem);

    return ret;
}

int WorkerTask::GetMessageNumber()
{
    return m_mailbox.Size();
}

MessageBase* WorkerTask::GetMessage()
{
    MessageBase* msg;

    sem_wait(&m_sem);

    msg = m_mailbox.PopFront();

    return msg;
}

void WorkerTask::SetStopState(bool shouldStop)
{
    pthread_spin_lock(&m_stoplock);
    m_shouldStop = shouldStop;
    pthread_spin_unlock(&m_stoplock);
}

bool WorkerTask::ShouldStop()
{
    bool stop;
    pthread_spin_lock(&m_stoplock);
    stop = m_shouldStop;
    pthread_spin_unlock(&m_stoplock);

    return stop;
}


void WorkerTask::StopRunning()
{
    SetStopState(true);
}

bool WorkerTask::Run()
{
    bool stop;

    while(1)
    {

        m_isRuning = false;

        stop = ShouldStop();
                 
        if (stop) break;

        MessageBase* msg = GetMessage();

        if (msg == NULL)continue;

        m_isRuning = true;
        msg->Run();

        delete msg;
    }

    SetStopState(false);
    return true;
}


/*
 *    Worker
 *
 */


Worker::Worker()
:Thread()
,m_workerTask()
{
   m_task = &m_workerTask;
}


Worker::~Worker()
{
}

