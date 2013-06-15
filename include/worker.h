#ifndef _WORKER_THREAD_H_
#define _WORKER_THREAD_H_

#include "thread.h"
#include "defs.h"
#include "ITask.h"
#include "SpinlockQueue.h"

#include <semaphore.h>

class Worker;

class WorkerBodyBase: public ITask
{
    public:

        WorkerBodyBase(Worker*worker = NULL);
        virtual ~WorkerBodyBase();

        bool PostTask(ITask*);
        void ClearAllTask();

        virtual int GetContainerSize() = 0;
        int GetTaskNumber();

        virtual bool StopRunning();

        bool IsRunning() const;
        int  TaskDone() const { return m_done; }
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

        //this is used to put a exit task in the msg queue.
        //so that we can exit the worker loop in an elegant way.
        //be aware, using this function will put the task in run
        //immediatly after current task is finished.
        //this is for INTERNAL usage only.
        virtual bool   PushTaskToContainerFront(ITask*) = 0;

        inline void SignalPost();
        inline void SignalConsume();
        inline bool SignalConsumeTimeout(int);
        inline bool TryConsume();
        inline int  Notify();
        inline bool PostExit();


    private:

        //disable overloading from child
        void Run();

        //get task from mailbox
        //may block when there is no task.
        //caller take responsibility to free the task.
        ITask* GetRunTask();

        bool CheckExit(ITask*);


        //is running task.
        volatile bool m_isRuning;

        //timeout for sem_timewait
        const int m_timeout;
        
        //record how many time we send notify without 
        //response, when this variable meet a certain threshhold.
        //shutdown timeout operation.
        volatile int m_req;
        const int m_reqThreshold;

        //semaphore for waiting for task.
        sem_t m_sem;

        //task processed.
        volatile int m_done;

        //notify thread pool I am free now, send me some tasks
        volatile bool m_notify;
        Worker* m_worker;
};


class WorkerBody: public WorkerBodyBase
{
    public:

        WorkerBody(Worker* worker = NULL, int maxMsgSize = DEFAULT_WORKER_TASK_MSG_SIZE);
        ~WorkerBody();

        virtual int  GetContainerSize();

    protected:

        virtual void HandleTask(ITask*);
        virtual bool HasTask();
        virtual ITask* GetTaskFromContainer();
        virtual bool PushTaskToContainer(ITask*);
        virtual bool PushTaskToContainerFront(ITask*);

    private:

        SpinlockWeakPriorityQueue<ITask*> m_mailbox;
};

class WorkerManagerBase
{
    public:

        WorkerManagerBase() {}
        virtual ~WorkerManagerBase() {}

        virtual int SetWorkerNotify(Worker*) = 0;
};

class Worker:public Thread
{
    public:

        Worker(WorkerManagerBase* manager = NULL, int id = -1
                ,int maxMsgSize = DEFAULT_WORKER_TASK_MSG_SIZE);

        Worker(WorkerBodyBase*, int id = -1, WorkerManagerBase* man = NULL);

        ~Worker();

        virtual bool IsRunning() const { return m_WorkerBody->IsRunning(); }
        bool StopWorking(bool join = true);
        bool PostTask(ITask* msg) { return m_WorkerBody->PostTask(msg); }
        
        virtual int  GetTaskNumber() { return m_WorkerBody->GetTaskNumber(); }

        void EnableNotify(bool enable = true) { m_WorkerBody->EnableNotify(enable); }

        bool StartWorking();

        int GetWorkerId() const { return m_id; } 

        int Notify(); 

        int TaskDone() const { return m_WorkerBody->TaskDone(); }

    protected:

        //disable setting task. 
        //this is a special thread specific to a worker thread.
        //It should not be changed externally.
        using Thread::SetTask;
        using Thread::Start;

        const int m_id;
        WorkerManagerBase* m_manager;
        WorkerBodyBase* m_WorkerBody;
};

#endif

