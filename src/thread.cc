#include "thread.h"
#include <iostream.h>
using namespace std;

#define slog(s,...)

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


void* dummy_proc(void*)
{
    slog("dummy proc , gogogo\n");
    slog("dummy proc , gogogo\n");
}


bool Thread::Start()
{
    if (m_threadStarted) return false;

    m_threadStarted = true;
    int status = pthread_attr_init(&m_attr);
    if (status != 0) return false;

    if (m_detachable)
    {
        status = pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_JOINABLE);
    }
    else
    {
        status = pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_DETACHED);
    }

    if (status != 0) return false;

    status = pthread_create(&m_tid,&m_attr,Thread::Run,static_cast<void*>(this));
    
    if(status)
       slog("error:%d %s\n",status,strerror(status));

    return status == 0;
}


void* Thread::Run(void*arg)
{
    Thread* thread  = static_cast<Thread*>(arg);
    ITask * task    = thread->GetTask();

    slog("thread::Run()\n");

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



