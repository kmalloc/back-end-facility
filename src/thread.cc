#include "thread.h"


Thread::Thread(ITask* task,bool detachable)
    :m_tid(-1)
    ,m_task(task)
    ,m_threadStarted(false)
    ,m_detachable(detachable)
{

}

Thread::~Thread()
{
    if(m_threadStarted) pthread_attr_destroy(&m_attr);
}


bool Thread::start()
{
    if (m_threadStarted) return false;

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

    status = pthread_create(&m_tid,&m_attr,Thread::run,static_cast<void*>(this));
    
    return status;
}


void* Thread::run(void*arg)
{
    Thread* thread  = static_cast<Thread*>(arg);
    ITask * task    = thread->get_task();

    if (task == NULL) return NULL;

    task->run();

    return task;
}



int Thread::join()
{
    if (!m_detachable) return -1;

    return pthread_join(m_tid,NULL);
}


bool Thread::set_detachable(bool enable)
{
    if (m_threadStarted)return false;

    m_detachable = enable;
}



