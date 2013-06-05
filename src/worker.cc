#include "worker.h"
#include "message.h"

#include <vector>
#include <queue>


using namespace std;

/*
 *     WorkerTask
 */

WorkerTask::WorkerTask(int msgSize)
:m_MaxSize(msgSize), m_isRuning(false), m_shouldStop(false)
{
    pthread_spin_init(&m_lock,0);
    pthread_spin_init(&m_stoplock,0);
    sem_init(&m_sem,0,0);
}



WorkerTask::~WorkerTask()
{
    while (!m_mailbox.empty())
    {
        MessageBase*msg = m_mailbox.front();
        m_mailbox.pop();
        delete(msg);
    }
}



bool WorkerTask::PostMessage(MessageBase* message)
{
    bool full = true;

    pthread_spin_lock(&m_lock);
    full = m_mailbox.size() > m_MaxSize;

    if (!full)
    {
        m_mailbox.push(message);
        pthread_spin_unlock(&m_lock);
        sem_post(&m_sem);
    }
    else
    {
        pthread_spin_unlock(&m_lock);
    }

    return !full;
}

int WorkerTask::GetMessageNumber()
{
    int count = -1;
    
    pthread_spin_lock(&m_lock);
    count = m_mailbox.size();
    pthread_spin_unlock(&m_lock);

    return count;
}

MessageBase* WorkerTask::GetMessage()
{
    MessageBase* msg;

    sem_wait(&m_sem);

    pthread_spin_lock(&m_lock);
    msg = m_mailbox.front();
    m_mailbox.pop();
    pthread_spin_unlock(&m_lock);

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

