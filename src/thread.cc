#include "thread.h"
#include "log.h"
#include "ITask.h"

#include <string.h>
#include <errno.h>


Thread::Thread(ITask* task,bool detachable)
:m_tid(-1)
,m_task(task)
,m_busy(false)
,m_detachable(detachable)
{
}

Thread::~Thread()
{
    pthread_attr_destroy(&m_attr);
}


void* dummy_proc(void*)
{
    slog("dummy proc , gogogo\n");
    slog("dummy proc , gogogo\n");
    return NULL;
}


bool Thread::Start()
{
    if (m_busy || m_task == NULL) return false;

    int status = pthread_attr_init(&m_attr);
    if (status != 0) return false;

    if (m_detachable)
    {
        status = pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_DETACHED);
    }
    else
    {
        status = pthread_attr_setdetachstate(&m_attr, PTHREAD_CREATE_JOINABLE);
    }

    if (status != 0) return false;

    m_busy = true;

    status = pthread_create(&m_tid,&m_attr,Thread::Run,static_cast<void*>(this));

    if (status) slog("error:%d %s\n",status,strerror(status));

    return status == 0;
}


void* Thread::Run(void*arg)
{
    Thread* thread  = static_cast<Thread*>(arg);
    ITask * task    = thread->m_task;

    if (task == NULL) return NULL;

    task->Run();

    thread->m_busy = false;

    return task;
}



int Thread::Join()
{
    if (m_detachable) return -1;

    return pthread_join(m_tid,NULL);
}


bool Thread::SetDetachable(bool enable)
{
    if (m_busy)return false;

    m_detachable = enable;
    return true;
}



