#ifndef _WORKER_THREAD_H_
#define _WORKER_THREAD_H_

#include "thread.h"
#include "defs.h"
#include "ITask.h"
#include "SpinlockQueue.h"

#include <semaphore.h>

class MessageBase;

class WorkerTask: public ITask
{
    public:

        WorkerTask(int maxMsgSize = DEFAULT_WORKER_TASK_MSG_SIZE);
        virtual ~WorkerTask();

        bool PostMessage(MessageBase*);

        void StopRunning();
        bool IsRunning() const {return m_isRuning;}
        int  GetMessageNumber();

    protected:

        virtual void Run();

        //get message from mailbox
        //may block when there is no message.
        //caller take responsibility to free the message.
        MessageBase* GetMessage();

        inline void SetStopState(bool shouldStop);

    private:

        volatile bool m_isRuning;
        volatile bool m_shouldStop;

        SpinlockQueue<MessageBase*> m_mailbox;
        sem_t m_sem;
};


class Worker:public Thread
{
    public:

        Worker();
        ~Worker();

        virtual bool IsRunning(){ return m_workerTask.IsRunning(); }
        void StopRunning() { m_workerTask.StopRunning(); }
        bool PostMessage(MessageBase* msg) { return m_workerTask.PostMessage(msg); }
        int  GetMessageNumber() { return m_workerTask.GetMessageNumber(); }

    protected:

        //disable setting task. this is a special thread specific to a worker thread.
        //It should not be changed externally.
        virtual ITask* SetTask(ITask*) { return NULL; }

        WorkerTask m_workerTask;
};

#endif

