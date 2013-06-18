#ifndef _THREAD_BASE_H_
#define _THREAD_BASE_H_

#include "misc/noncopyable.h"
#include <pthread.h>

class ITask;


class Thread: public noncopyable
{

    public:

        Thread(ITask* = NULL,bool detachable = false);
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
        
        volatile bool m_busy;
        volatile bool m_detachable;
};



#endif

