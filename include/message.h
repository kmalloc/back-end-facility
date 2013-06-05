#ifndef _MESSAGE_H_
#define _MESSAGE_H_

#include "ITask.h"


class MessageBase: public ITask 
{
    public:

        MessageBase(){}
        virtual ~MessageBase(){}


        bool Run() {}

        virtual void   PrepareMessage();
};


#endif

