#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include "ITask.h"


class MessageBase: public ITask 
{
    public:

        MessageBase(){}

        virtual ~MessageBase(){}

        virtual void Run() {}

        virtual void   PrepareMessage();

        int Priority() const { return m_priority;}

    private:

        int m_priority;
};


#endif

