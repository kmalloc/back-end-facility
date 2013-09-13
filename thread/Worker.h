#ifndef _WORKER_THREAD_H_
#define _WORKER_THREAD_H_

#include "Thread.h"
#include "ITask.h"
#include "sys/Defs.h"
#include "misc/SpinlockQueue.h"

#include <semaphore.h>

class Worker;

class WorkerBodyBase: public ITask
{
    public:

        WorkerBodyBase(Worker*worker = NULL);
        virtual ~WorkerBodyBase();

        bool PostTask(ITask*);
        void ClearAllTask();

        int GetTaskNumber();
        virtual int GetContainerSize() = 0;

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

        virtual void PreHandleTask() {}
        virtual void HandleTask(ITask*) = 0;
        virtual bool  HasTask() = 0; 

        //be aware: this function may block if there is no task
        virtual ITask* GetTaskFromContainer() = 0;
        virtual bool   PushTaskToContainer(ITask*) = 0;

        inline void SignalPost();
        inline void SignalConsume();
        inline bool SignalConsumeTimeout(int);
        inline bool TryConsume();
        inline int  Notify();
        inline bool PostExit();

    private:

        //disable overloading from child
        void Run();

        //get task from mailbox or m_cmd
        //may block when there is no task.
        //caller take responsibility to free the task.
        //return false if get task from m_cmd.
        bool GetRunTask(ITask*& task);

        //set up flag to make worker loop exit;
        //will be called in worker thread only.
        inline void SetExitState();

        inline bool PostInternalCmd(ITask*);
        inline bool PushInternalCmd(ITask*);
        inline ITask* GetInternalCmd();
        inline void ProcessInternalCmd(ITask*);

        //is running task.
        volatile bool m_isRuning;

        //timeout for sem_timewait
        const int m_timeout;
        
        const int m_reqThreshold;

        //semaphore for waiting for task.
        sem_t m_sem;

        //task processed.
        volatile int m_done;

        //notify thread pool I am free now, send me some tasks
        volatile bool m_notify;
        Worker* m_worker;

        //note this is for internal usage only.
        //supporting worker internal activities: exit.
        SpinlockQueue<ITask*> m_cmd;

        //exit flag.
        //this variable will be accessed on in the worker thread.
        //It will be set by DummyExitTask only.
        bool m_ShouldStop;

        friend class DummyExitTask;
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

    private:

        SpinlockWeakPriorityQueue<ITask*> m_mailbox;
};

class WorkerManagerBase
{
    public:

        WorkerManagerBase() {}
        virtual ~WorkerManagerBase() {}

    protected:

        virtual int SetWorkerNotify(Worker*) = 0;

        friend class Worker;
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

        virtual bool StartWorking();

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

