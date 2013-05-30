#ifndef _WORKER_THREAD_H_
#define _WORKER_THREAD_H_

#include "thread.h"
#include "ITask.h"

class WorkerTask: public ITask
{
    public:
        WorkerTask();
        ~WorkerTask();

    protected:

        virtual bool Run();

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

