#ifndef _MESSAGE_H_
#ifndef _MESSAGE_H_

#include "ITask.h"


class MessageBase 
{
    public:

        TaskMessage(): m_task(NULL) {}
        virtual ~TaskMessage(){}

        ITask* GetTask() const { return m_task; }
        void   SetTask(ITask* task) { m_task = task; }

        virtual void   PrepareMessage();

    private:
        
        ITask* m_task;
};


#endif

