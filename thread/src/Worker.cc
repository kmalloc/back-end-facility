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

int WorkerBody::GetContainerSize() const
{
    return mailbox_.Size();
}

bool WorkerBody::HasTask()
{
    return !mailbox_.IsEmpty();
}

bool WorkerBody::HandleTask(ITask* task)
{
    task->Run();
    return true;
}

/*
 *    Worker
 *
 */

Worker::Worker(WorkerManagerBase* man, int id, int maxMsgSize)
    :Thread(NULL), id_(id), manager_(man)
{
   WorkerBody_ = new WorkerBody(this, maxMsgSize);
   SetTask(WorkerBody_);
}

Worker::Worker(WorkerBodyBase* task, int id, WorkerManagerBase* man)
    :Thread(task), id_(id), manager_(man), WorkerBody_(task)
{
}

Worker::~Worker()
{
    delete task_;
}

bool Worker::StartWorking()
{
    return Thread::Start();
}

int Worker::Notify(int type)
{
    if (manager_)
    {
        return manager_->SetWorkerNotify(this, type);
    }

    return 0;
}

bool Worker::StopWorking(bool join)
{
    bool ret =  WorkerBody_->StopRunning();

    if (join && ret) ret = Join();

    return ret;
}

