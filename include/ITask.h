#ifndef _I_TASK_H_
#define _I_TASK_H_

#include <assert.h>

enum TaskPriority
{
    TP_EXTREME,
    TP_HIGH,
    TP_NORMAL,
    TP_LOW
};

enum TaskFlag
{
    TF_NULL,
    TF_EXIT,
    TF_POOL_WORKER_REQUEST,
};

class ITask
{
    public:

        ITask(TaskPriority prio = TP_NORMAL): m_internal(TF_NULL), m_priority(prio){}
        virtual ~ITask(){}
        virtual void Run()=0;

        TaskPriority Priority() const { return m_priority; }
        TaskPriority SetPriority(TaskPriority prio)
        {
            TaskPriority old = m_priority;
            m_priority = prio;
            return old;
        }

        TaskFlag GetInternalFlag() const { return m_internal; }

    protected:

        TaskPriority m_priority;

        //special variable for internal usage.
        //indicate special task.
        //eg, make function run.
        TaskFlag m_internal;
};



class DummyExitTask: public ITask
{
    public:

        DummyExitTask(): ITask(TP_LOW) { m_internal = TF_EXIT; }

        virtual ~DummyExitTask() {}

        virtual void Run() { assert(0); }

};

#endif

