#ifndef _WORKER_BODYBASE_H_
#define _WORKER_BODYBASE_H_

#include "ITask.h"
#include "sys/Defs.h"
#include "misc/NonCopyable.h"

#include <semaphore.h>

class NotifyerBase
{
    public:

        virtual int Notify() = 0;
};

class WorkerBodyBase: public ITask, public noncopyable
{
    public:

        WorkerBodyBase(NotifyerBase* notifyer = NULL);
        virtual ~WorkerBodyBase();

        bool PostTask(ITask*);
        void ClearAllTask();

        int GetTaskNumber();
        virtual int GetContainerSize() = 0;

        virtual bool StopRunning();

        bool IsRunning() const;
        int  TaskDone() const { return m_done; }
        void EnableNotify(bool enable = true) { m_notify = enable; }

        // take care of calling this function.
        // multitasking-opertion on m_mailbox will
        // greatly reduce performance.
        // don't call it unless really necessary.
        ITask* TryGetTask();

    protected:

        virtual void PreHandleTask() {}
        virtual void HandleTask(ITask*) = 0;
        virtual bool  HasTask() = 0;

        // be aware: this function may block if there is no task
        virtual ITask* GetTaskFromContainer() = 0;
        virtual bool   PushTaskToContainer(ITask*) = 0;

        inline void SignalPost();
        inline void SignalConsume();
        inline bool SignalConsumeTimeout(int);
        inline bool TryConsume();
        inline int  Notify();
        inline bool PostExit();

    private:

        // disable overloading from child
        void Run();

        // get task from mailbox or m_cmd
        // may block when there is no task.
        // caller take responsibility to free the task.
        // return false if get task from m_cmd.
        bool GetRunTask(ITask*& task);

        // set up flag to make worker loop exit;
        // will be called in worker thread only.
        inline void SetExitState();

        inline bool PostInternalCmd(ITask*);
        inline bool PushInternalCmd(ITask*);
        inline ITask* GetInternalCmd();
        inline void ProcessInternalCmd(ITask*);

        // is running task.
        volatile bool m_isRuning;

        // timeout for sem_timewait
        const int m_timeout;

        const int m_reqThreshold;

        // semaphore for waiting for task.
        sem_t m_sem;

        // task processed.
        volatile int m_done;

        // notify thread pool I am free now, send me some tasks
        volatile bool m_notify;
        NotifyerBase* m_notifyer;

        // note this is for internal usage only.
        // supporting worker internal activities: exit.
        SpinlockQueue<ITask*> m_cmd;

        // exit flag.
        // this variable will be accessed on in the worker thread.
        // It will be set by DummyExitTask only.
        bool m_ShouldStop;

        friend class DummyExitTask;
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

#endif

