#ifndef _I_TASK_H_
#define _I_TASK_H_

enum TaskPriority
{
    TP_EXTREME,
    TP_HIGH,
    TP_NORMAL,
    TP_LOW
};

class ITask
{
    public:

        ITask(TaskPriority prio = TP_NORMAL): m_priority(prio){}
        virtual ~ITask(){}
        virtual void Run()=0;

        TaskPriority Priority() const { return m_priority; }
        TaskPriority SetPriority(TaskPriority prio)
        {
            TaskPriority old = m_priority;
            m_priority = prio;
            return old;
        }

    protected:

        TaskPriority m_priority;
};

#endif

