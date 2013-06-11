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

        bool   IsDetachable() const { return m_detachable; }
        bool   SetDetachable(bool enable);

        bool   Start();
        bool   Join(void** ret = NULL);
        bool   Cancel();

        virtual bool IsRunning() const { return m_busy; }

        virtual ITask* SetTask(ITask*task) { ITask* tmp = m_task; m_task = task; return tmp;}
        virtual const ITask* GetTask() const { return m_task; }
        
    protected:
        
        static void* Run(void*arg);        

        ITask* m_task;

    private:

        pthread_t m_tid;
        pthread_attr_t m_attr;
        
        volatile bool m_busy;
        bool m_detachable;
};



#endif

