#ifndef _WORKER_THREAD_H_
#define _WORKER_THREAD_H_

#include "thread.h"
#include "defs.h"
#include "ITask.h"
#include "SpinlockQueue.h"

#include <semaphore.h>

class MessageBase;

class WorkerTaskBase: public ITask
{
    public:

        WorkerTaskBase();
        virtual ~WorkerTaskBase();

        virtual bool PostMessage(MessageBase*) = 0;
        virtual int  GetMessageNumber() = 0;
        virtual void ClearAllMsg()=0;

        virtual void StopRunning();
        virtual bool IsRunning() const {return m_isRuning;}

    protected:

        virtual void Run()=0;

        //get message from mailbox
        //may block when there is no message.
        //caller take responsibility to free the message.
        virtual MessageBase* GetMessage() = 0;

        inline void SetStopState(bool shouldStop);
        inline void SignalPost();
        inline void SignalConsume();

        volatile bool m_isRuning;
        volatile bool m_shouldStop;

    private:

        sem_t m_sem;
};


class WorkerTask: public WorkerTaskBase
{
    public:

        WorkerTask(int maxMsgSize = DEFAULT_WORKER_TASK_MSG_SIZE);
        ~WorkerTask();

        virtual bool PostMessage(MessageBase*);
        virtual int  GetMessageNumber();
        virtual void ClearAllMsg();

    protected:

        virtual void Run();
        virtual MessageBase* GetMessage();
        
    private:

        SpinlockQueue<MessageBase*> m_mailbox;
};


class Worker:public Thread
{
    public:

        Worker(int maxMsgSize = DEFAULT_WORKER_TASK_MSG_SIZE);
        ~Worker();

        virtual bool IsRunning(){ return m_workerTask->IsRunning(); }
        void StopRunning() { m_workerTask->StopRunning(); }
        bool PostMessage(MessageBase* msg) { return m_workerTask->PostMessage(msg); }
        int  GetMessageNumber() { return m_workerTask->GetMessageNumber(); }


        bool StartWorking();

    protected:

        Worker(WorkerTaskBase*);

        //disable setting task. this is a special thread specific to a worker thread.
        //It should not be changed externally.
        using Thread::SetTask;
        using Thread::Start;

        WorkerTaskBase* m_workerTask;
};

#endif

