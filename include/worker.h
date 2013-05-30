#ifndef _WORKER_THREAD_H_
#define _WORKER_THREAD_H_

#include "thread.h"
#include "defs.h"
#include "ITask.h"

#include "semaphore.h"

class MessageBase;

class WorkerTask: public ITask
{
    public:
        WorkerTask(int maxMsgSize = DEFAULT_WORKER_TASK_MSG_SIZE);
        virtual ~WorkerTask();

        bool PutMessage(MessageBase*);

    protected:

        virtual bool Run();

        //get message from mailbox
        //may block when there is no message.
        //caller take responsibility to free the message.
        MessageBase* GetMessage();

    private:

        const int m_MaxSize;
        sem_t m_sem;
        pthread_spinlock_t m_lock;
        std::queue<MessageBase*> m_mailbox;
};


class Worker:public Thread
{
    public:
        Worker();
        ~Worker();
        
        virtual bool IsRunning();

    protected:

        //disable set task. this is a special thread specific to a worker thread.
        //It should not be changed externally.
        virtual ITask* SetTask(ITask*){ return NULL;}

};

#endif

