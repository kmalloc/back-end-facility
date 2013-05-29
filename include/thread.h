#ifndef _THREAD_BASE_H_
#define _THREAD_BASE_H_

#include <pthread.h>
#include "noncopyable.h"

class ITask;


class Thread: public noncopyable
{

    public:

        Thread(ITask* = NULL,bool detachable = true);
        virtual ~Thread();

        ITask* SetTask(ITask*task) { ITask* tmp = m_task; m_task = task; return tmp;}
        bool   IsDetachable() const { return m_detachable; }
        bool   SetDetachable(bool enable);

        bool   Start();
        int    Join();
        bool   IsStarted() const { return m_threadStarted; }

        ITask* GetTask() const { return m_task; }
        
    protected:
        static void* Run(void*arg);        

    private:
        pthread_t m_tid;
        pthread_attr_t m_attr;
        
        ITask* m_task;
        bool m_threadStarted;
        bool m_detachable;
};



#endif

