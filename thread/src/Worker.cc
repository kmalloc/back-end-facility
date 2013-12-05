#include "Worker.h"
#include "sys/Log.h"

/*
 * WorkerBody
 */

WorkerBody::WorkerBody(NotifyerBase* notifyer, int maxMsgSize)
    :WorkerBodyBase(notifyer), mailbox_(maxMsgSize)
{
}

WorkerBody::~WorkerBody()
{
    WorkerBodyBase::ClearAllTask();
}

ITask* WorkerBody::GetTaskFromContainer()
{
    ITask* task;
    if (mailbox_.PopFront(&task)) return task;

    return NULL;
}

bool WorkerBody::PushTaskToContainer(ITask* task)
{
    return mailbox_.PushBack(task);
}

int WorkerBody::GetContainerSize()
{
    return mailbox_.Size();
}

bool WorkerBody::HasTask()
{
    return !mailbox_.IsEmpty();
}

void WorkerBody::HandleTask(ITask* task)
{
    task->Run();
    delete task;
}

/*
 *    Worker
 *
 */

Worker::Worker(WorkerManagerBase* man, int id, int maxMsgSize)
    :Thread(), id_(id), manager_(man)
{
   task_ = WorkerBody_ = new WorkerBody(this,maxMsgSize);
}

Worker::Worker(WorkerBodyBase* task, int id, WorkerManagerBase* man)
    :Thread(), id_(id), manager_(man)
{
    task_ = WorkerBody_ = task;
}

Worker::~Worker()
{
    delete task_;
}

bool Worker::StartWorking()
{
    return Thread::Start();
}

int Worker::Notify()
{
    if (manager_)
    {
        return manager_->SetWorkerNotify(this);
    }

    return 0;
}

bool Worker::StopWorking(bool join)
{
    bool ret =  WorkerBody_->StopRunning();

    if (join && ret) ret = Join();

    return ret;
}

