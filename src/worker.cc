#include "worker.h"
#include "message.h"

#include <queue.h>


using namespace std;


WorkerTask::WorkerTask(int msgSize)
    :m_MaxSize(msgSize)
{
    pthread_spin_init(&m_lock,0);
    sem_init(&m_sem,0,m_MaxSize);
}



WorkerTask::~WorkerTask()
{

    queue<MessageBase*>::iterator it = m_mailbox.begin();

    for (; it != m_mailbox.end();++it)
    {
        delete (*it);
    }
}



bool WorkerTask::PutMessage(MessageBase*message)
{
    bool full = true;

    pthread_spin_lock(&m_lock);
    full = m_mailbox.size() > m_MaxSize;
    pthread_spin_unlock(&m_lock);

    if (!full)
    {
        m_mailbox.push_back(message);
        sem_post(&m_sem);
    }

    return !full;
}



MessageBase* WorkerTask::GetMessage()
{
    MessageBase* msg = NULL;
    
    sem_wait(&m_sem);

    pthread_spin_lock(&m_lock);
    msg = m_mailbox.pop_front();
    pthread_spin_unlock(&m_lock);

    return msg;
}




