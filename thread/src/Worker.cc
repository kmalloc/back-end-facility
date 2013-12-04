#include "Worker.h"
#include "sys/Log.h"

/*
 * WorkerBody
 */

WorkerBody::WorkerBody(NotifyerBase* notifyer, int maxMsgSize)
    :WorkerBodyBase(notifyer), m_mailbox(maxMsgSize)
{
}

WorkerBody::~WorkerBody()
{
    WorkerBodyBase::ClearAllTask();
}

ITask* WorkerBody::GetTaskFromContainer()
{
    ITask* task;
    if (m_mailbox.PopFront(&task)) return task;

    return NULL;
}

bool WorkerBody::PushTaskToContainer(ITask* task)
{
    return m_mailbox.PushBack(task);
}

int WorkerBody::GetContainerSize()
{
    return m_mailbox.Size();
}

bool WorkerBody::HasTask()
{
    return !m_mailbox.IsEmpty();
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
    :Thread(), m_id(id), m_manager(man)
{
   m_task = m_WorkerBody = new WorkerBody(this,maxMsgSize);
}

Worker::Worker(WorkerBodyBase* task, int id, WorkerManagerBase* man)
    :Thread(), m_id(id), m_manager(man)
{
    m_task = m_WorkerBody = task;
}

Worker::~Worker()
{
    delete m_task;
}

bool Worker::StartWorking()
{
    return Thread::Start();
}

int Worker::Notify()
{
    if (m_manager)
    {
        return m_manager->SetWorkerNotify(this);
    }

    return 0;
}

bool Worker::StopWorking(bool join)
{
    bool ret =  m_WorkerBody->StopRunning();

    if (join && ret) ret = Join();

    return ret;
}

