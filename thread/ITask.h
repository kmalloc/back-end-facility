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

        ITask(TaskPriority prio = TP_NORMAL): priority_(prio){}
        virtual ~ITask(){}
        virtual void Run()=0;

        TaskPriority Priority() const { return priority_; }
        TaskPriority SetPriority(TaskPriority prio)
        {
            TaskPriority old = priority_;
            priority_ = prio;
            return old;
        }

    protected:

        TaskPriority priority_;
};

#endif

