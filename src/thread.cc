#include "thread.h"



void ITask::SetLoop(bool loop) 
{
    m_loop = loop;
}

void ITask::StopLoop() 
{ 
    m_loop = false; 
}

Thread::Thread(ITask* task,bool detachable)
    :m_tid(-1)
    ,m_task(task)
    ,m_threadStarted(false)
    ,m_detachable(detachable)
{

}

Thread::~Thread()
{
    if (m_threadStarted) pthread_attr_destroy(&m_attr);
}


bool Thread::Start()
{
    if (m_threadStarted) return false;

    m_threadStarted = true;
    int status = pthread_attr_init(&m_attr);
    if (status != 0) return false;

    if (m_detachable)
    {
        pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_JOINABLE);
    }
    else
    {
        pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_DETACHED);
    }

    if (status != 0) return false;

    status = pthread_create(&m_tid,&m_attr,Thread::Run,static_cast<void*>(this));
    
    return status;
}


void* Thread::Run(void*arg)
{
    Thread* thread  = static_cast<Thread*>(arg);
    ITask * task    = thread->GetTask();

    do
    {
        if (task == NULL) return NULL;

        task->Run();

    } while(task->Loop());

    thread->m_threadStarted = false;

    return task;
}



int Thread::Join()
{
    if (!m_detachable) return -1;

    return pthread_join(m_tid,NULL);
}


bool Thread::SetDetachable(bool enable)
{
    if (m_threadStarted)return false;

    m_detachable = enable;
    return true;
}



