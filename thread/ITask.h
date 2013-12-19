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

        ITask(bool autoDel = true, TaskPriority prio = TP_NORMAL): affinity_(-1), deleteAfterRun_(autoDel), priority_(prio){}
        virtual ~ITask(){}
        virtual void Run()=0;

        TaskPriority Priority() const { return priority_; }
        TaskPriority SetPriority(TaskPriority prio)
        {
            TaskPriority old = priority_;
            priority_ = prio;
            return old;
        }

        void SetAffinity(int aff) { affinity_ = aff; }
        int  GetAffinity() const { return affinity_; }
        bool ShouldDelete() const { return deleteAfterRun_; }

    protected:

        int affinity_; // pid, used to bind Itask to specific worker
        bool deleteAfterRun_;
        TaskPriority priority_;
};

#endif

