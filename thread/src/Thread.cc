#include "Thread.h"
#include "ITask.h"
#include "sys/Log.h"

#include <string.h>
#include <errno.h>

Thread::Thread(ITask* task, bool detachable)
    :m_task(task)
    ,m_tid(0)
    ,m_busy(false)
    ,m_detachable(detachable)
{
}

Thread::~Thread()
{
    if (m_busy) pthread_cancel(m_tid);
}

bool Thread::Start()
{
    if (m_busy || m_task == NULL) return false;

    pthread_attr_t attr;
    int status = pthread_attr_init(&attr);
    if (status != 0)
    {
        slog(LOG_ERROR, "init attr fail:%d %s\n",status,strerror(status));
        return false;
    }

    if (m_detachable)
    {
        status = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    }
    else
    {
        status = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    }

    if (status != 0)
    {
        slog(LOG_ERROR, "set detachable fail:%d %s\n",status,strerror(status));
        return false;
    }

    m_busy = true;

    status = pthread_create(&m_tid,&attr, Thread::RunTask, static_cast<void*>(this));

    pthread_attr_destroy(&attr);

    if (status) slog(LOG_FATAL, "error:%d %s\n", status, strerror(status));

    return status == 0;
}

void* Thread::RunTask(void*arg)
{
    Thread* thread  = static_cast<Thread*>(arg);
    ITask * task    = thread->m_task;

    if (task == NULL) return NULL;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    task->Run();

    thread->m_busy = false;

    return task;
}

bool Thread::Join(void** ret)
{
    if (m_detachable) return false;

    return pthread_join(m_tid,ret) == 0;
}

bool Thread::Cancel()
{
    if (!m_busy) return true;
    return pthread_cancel(m_tid) == 0;
}

bool Thread::SetDetachable(bool enable)
{
    if (m_busy) return false;

    m_detachable = enable;
    return true;
}

///thread base
//

ThreadBase::ThreadBase(bool detachable /* = false*/)
    :Thread(NULL, detachable), m_task(this)
{
    Thread::SetTask(&m_task);
}

ThreadBase::~ThreadBase()
{
}

