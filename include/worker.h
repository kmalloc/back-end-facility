#ifndef _WORKER_THREAD_H_
#define _WORKER_THREAD_H_

#include "thread.h"
#include "defs.h"
#include "ITask.h"

#include <queue>
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

        virtual bool Run();

        //get message from mailbox
        //may block when there is no message.
        //caller take responsibility to free the message.
        MessageBase* GetMessage();

        inline bool ShouldStop();
        inline void SetStopState(bool shouldStop);

    private:

        const int m_MaxSize;
        volatile bool m_isRuning;
        volatile bool m_shouldStop;

        sem_t m_sem;
        pthread_spinlock_t m_lock;
        pthread_spinlock_t m_stoplock;
        std::queue <MessageBase*> m_mailbox;
};


class Worker:public Thread
{
    public:
        Worker();
        ~Worker();

        virtual bool IsRunning(){ return m_workerTask.IsRunning();}
        void StopRunning() { m_workerTask.StopRunning();}
        bool PostMessage(MessageBase* msg) { return m_workerTask.PostMessage(msg);}
        int  GetMessageNumber() { return m_workerTask.GetMessageNumber();}

    protected:

        //disable set task. this is a special thread specific to a worker thread.
        //It should not be changed externally.
        virtual ITask* SetTask(ITask*) { return NULL;}

        WorkerTask m_workerTask;
};

#endif

