#include "Thread.h"
#include "ITask.h"
#include "sys/Log.h"
#include "sys/AtomicOps.h"

#include <string.h>
#include <errno.h>

Thread::Thread(ITask* task, bool detachable)
    :task_(task)
    ,tid_(0)
    ,busy_(0)
    ,detachable_(detachable)
{
}

Thread::~Thread()
{
    if (atomic_read(&busy_)) pthread_cancel(tid_);
}

bool Thread::Start()
{
    if (atomic_read(&busy_) || task_ == NULL) return false;

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

    status = pthread_create(&tid_, &attr, Thread::RunTask, static_cast<void*>(this));

    pthread_attr_destroy(&attr);
    if (status) slog(LOG_FATAL, "error:%d %s\n", status, strerror(status));

    return status == 0;
}

void* Thread::RunTask(void*arg)
{
    Thread* thread  = static_cast<Thread*>(arg);
    ITask * task    = thread->task_;

    if (task == NULL) return NULL;

    atomic_cas(&thread->busy_, 0, 1);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,NULL);
    task->Run();
    atomic_cas(&thread->busy_, 1, 0);

    return task;
}

bool Thread::Join(void** ret)
{
    if (detachable_) return false;

    return pthread_join(tid_,ret) == 0;
}

bool Thread::Cancel()
{
    if (!atomic_read(&busy_)) return true;

    return pthread_cancel(tid_) == 0;
}

bool Thread::SetDetachable(bool enable)
{
    if (atomic_read(&busy_)) return false;

    detachable_ = enable;
    return true;
}

bool Thread::IsRunning() const
{
    return atomic_read(&busy_);
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

