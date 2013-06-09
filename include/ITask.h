#ifndef _I_TASK_H_
#define _I_TASK_H_

class ITask
{
    public:
      ITask(){}
      virtual ~ITask(){}
      virtual void Run()=0;
};

#endif

