#ifndef _WORKER_THREAD_H_
#define _WORKER_THREAD_H_

#include "thread.h"
#include "defs.h"
#include "ITask.h"
#include "SpinlockQueue.h"

#include <semaphore.h>

class WorkerBodyBase: public ITask
{
    public:

        WorkerBodyBase();
        virtual ~WorkerBodyBase();

        virtual int  GetTaskNumber() = 0;

        void ClearAllTask();
        void StopRunning();
        bool IsRunning() const {return m_isRuning;}
        void EnableNotify(bool enable = true) { m_notify = enable; }

        //take care of calling this function.
        //multitasking-opertion on m_mailbox will 
        //greatly reduce performance.
        //don't call it unless really necessary.
        ITask* TryGetTask();

    protected:

        virtual void HandleTask(ITask*) = 0;
        inline bool  HasTask() const = 0; 

        //be aware: this function may block if there is no task
        inline virtual ITask* GetTaskFromContainer() = 0;
        inline virtual bool   PushTaskToContainer() = 0;

        //get task from mailbox
        //may block when there is no task.
        //caller take responsibility to free the task.
        ITask* GetTask();

        inline void SetStopState(bool shouldStop);
        inline void SignalPost();
        inline void SignalConsume();
        inline bool TryConsume();
        inline int  Notify();

        volatile bool m_isRuning;
        volatile bool m_shouldStop;


    private:

        void Run();

        sem_t m_sem;
        bool m_notify;
};


class WorkerBody: public WorkerBodyBase
{
    public:

        WorkerBody(int maxMsgSize = DEFAULT_WORKER_TASK_MSG_SIZE);
        ~WorkerBody();

        virtual int  GetTaskNumber();

    protected:

        virtual void HandleTask();
        inline bool HasTask();
        inline virtual ITask* GetTaskFromContainer();
        inline virtual bool   PushTaskToContainer();

    private:

        SpinlockQueue<ITask*> m_mailbox;
};


class Worker:public Thread
{
    public:

        Worker(ThreadPool* pool = NULL, int id = -1
                ,int maxMsgSize = DEFAULT_WORKER_TASK_MSG_SIZE);

        ~Worker();

        virtual bool IsRunning(){ return m_WorkerBody->IsRunning(); }
        void StopRunning() { m_WorkerBody->StopRunning(); }
        bool PostTask(ITask* msg) { return m_WorkerBody->PostTask(msg); }
        int  GetTaskNumber() { return m_WorkerBody->GetTaskNumber(); }

        bool StartWorking();

        int GetWorkerId const { return m_id; } 

    protected:

        Worker(WorkerBodyBase*);

        //disable setting task. 
        //this is a special thread specific to a worker thread.
        //It should not be changed externally.
        using Thread::SetTask;
        using Thread::Start;

        const int m_id;
        ThreadPool* m_pool;
        WorkerBodyBase* m_WorkerBody;
};

#endif

