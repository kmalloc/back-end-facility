#ifndef _WORKER_THREAD_H_
#define _WORKER_THREAD_H_

#include "Thread.h"
#include "WorkerBodyBase.h"

#include "misc/SpinlockQueue.h"
#include "misc/NonCopyable.h"

class WorkerBody: public WorkerBodyBase
{
    public:

        WorkerBody(NotifyerBase* notifyer = NULL, int maxMsgSize = DEFAULT_WORKER_TASK_MSG_SIZE);
        ~WorkerBody();

    protected:

        virtual int GetContainerSize() const;

        virtual bool HandleTask(ITask*);
        virtual bool HasTask();
        virtual ITask* GetTaskFromContainer();
        virtual bool PushTaskToContainer(ITask*);

    private:

        // this queue can be replaced with other container that supports identical interface.
        // better replace it with lock free container/list.
        SpinlockWeakPriorityQueue<ITask*> mailbox_;
};

class Worker: public Thread, public NotifyerBase
{
    public:

        Worker(WorkerManagerBase* manager = NULL, int id = -1
                ,int maxMsgSize = DEFAULT_WORKER_TASK_MSG_SIZE);

        Worker(WorkerBodyBase*, int id = -1, WorkerManagerBase* man = NULL);

        ~Worker();

        virtual bool IsRunning() const { return WorkerBody_->IsRunning(); }
        bool StopWorking(bool join = true);
        bool PostTask(ITask* msg) { return WorkerBody_->PostTask(msg); }

        virtual int GetTaskNumber() const { return WorkerBody_->GetTaskNumber(); }

        void EnableNotify(bool enable = true) { WorkerBody_->EnableNotify(enable); }

        virtual bool StartWorking();

        int GetWorkerId() const { return id_; }

        int TaskDone() const { return WorkerBody_->TaskDone(); }

        virtual int Notify(int type);

    protected:

        using Thread::Start;
        using Thread::SetTask;

        const int id_;
        WorkerManagerBase* manager_;
        WorkerBodyBase* WorkerBody_;
};

#endif

