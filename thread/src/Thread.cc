#include "Thread.h"
#include "ITask.h"
#include "sys/Log.h"

#include <string.h>
#include <errno.h>

Thread::Thread(ITask* task, bool detachable)
    :task_(task)
    ,tid_(0)
    ,busy_(false)
    ,detachable_(detachable)
{
}

Thread::~Thread()
{
    if (busy_) pthread_cancel(tid_);
}

bool Thread::Start()
{
    if (busy_ || task_ == NULL) return false;

    pthread_attr_t attr;
    int status = pthread_attr_init(&attr);
    if (status != 0)
    {
        slog(LOG_ERROR, "init attr fail:%d %s\n",status,strerror(status));
        return false;
    }

    if (detachable_)
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

    status = pthread_create(&tid_,&attr, Thread::RunTask, static_cast<void*>(this));

    pthread_attr_destroy(&attr);

    if (status) slog(LOG_FATAL, "error:%d %s\n", status, strerror(status));

    return status == 0;
}

void* Thread::RunTask(void*arg)
{
    Thread* thread  = static_cast<Thread*>(arg);
    ITask * task    = thread->task_;

    if (task == NULL) return NULL;

    thread->busy_ = true;

    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    task->Run();

    thread->busy_ = false;

    return task;
}

bool Thread::Join(void** ret)
{
    if (detachable_) return false;

    return pthread_join(tid_,ret) == 0;
}

bool Thread::Cancel()
{
    if (!busy_) return true;
    return pthread_cancel(tid_) == 0;
}

bool Thread::SetDetachable(bool enable)
{
    if (busy_) return false;

    detachable_ = enable;
    return true;
}

///thread base
ThreadBase::ThreadBase(bool detachable /* = false*/)
    :Thread(NULL, detachable), task_(this)
{
    Thread::SetTask(&task_);
}

ThreadBase::~ThreadBase()
{
}

