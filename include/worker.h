#ifndef _WORKER_THREAD_H_
#define _WORKER_THREAD_H_

#include "thread.h"
#include "defs.h"
#include "ITask.h"
#include "SpinlockQueue.h"

#include <semaphore.h>

class ThreadPool;
class Worker;

class WorkerBodyBase: public ITask
{
    public:

        WorkerBodyBase(Worker*worker = NULL);
        virtual ~WorkerBodyBase();

        bool PostTask(ITask*);
        void ClearAllTask();
        virtual int GetTaskNumber() = 0;

        virtual void StopRunning();
        bool IsRunning() const {return m_isRuning;}
        void EnableNotify(bool enable = true) { m_notify = enable; }

        //take care of calling this function.
        //multitasking-opertion on m_mailbox will 
        //greatly reduce performance.
        //don't call it unless really necessary.
        ITask* TryGetTask();

    protected:

        virtual void HandleTask(ITask*) = 0;
        virtual bool  HasTask() = 0; 

        //be aware: this function may block if there is no task
        virtual ITask* GetTaskFromContainer() = 0;
        virtual bool   PushTaskToContainer(ITask*) = 0;


        inline void SetStopState(bool shouldStop);
        inline void SignalPost();
        inline void SignalConsume();
        inline bool SignalConsumeTimeout(int);
        inline bool TryConsume();
        inline int  Notify();

        volatile bool m_isRuning;
        volatile bool m_shouldStop;

    private:

        //disable overloading from child
        void Run();

        //get task from mailbox
        //may block when there is no task.
        //caller take responsibility to free the task.
        ITask* GetRunTask();

        //timeout for sem_timewait
        const int m_timeout;
        
        //record how many time we send notify without 
        //response, when this variable meet a certain threshhold.
        //shutdown timeout operation.
        int m_req;
        const int m_reqThreshold;

        //semaphore for waiting for task.
        sem_t m_sem;

        //notify thread pool I am free now, send me some tasks
        bool m_notify;
        Worker* m_worker;
};


class WorkerBody: public WorkerBodyBase
{
    public:

        WorkerBody(Worker* worker = NULL, int maxMsgSize = DEFAULT_WORKER_TASK_MSG_SIZE);
        ~WorkerBody();

        virtual int  GetTaskNumber();

    protected:

        virtual void HandleTask(ITask*);
        virtual bool HasTask();
        virtual ITask* GetTaskFromContainer();
        virtual bool PushTaskToContainer(ITask*);

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

        void EnableNotify(bool enable = true) { m_WorkerBody->EnableNotify(enable); }

        bool StartWorking();

        int GetWorkerId() const { return m_id; } 

        int Notify(); 

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

