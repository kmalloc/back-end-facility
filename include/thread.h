#ifndef _THREAD_BASE_H_
#define _THREAD_BASE_H_

#include <pthread.h>
#include "noncopyable.h"


class ITask
{
    public:
      ITask();
      virtual ~ITask();

      virtual bool run()=0;
};


class Thread: public noncopyable
{

    public:
        Thread(ITask* = NULL,bool detachable = true);
        virtual ~Thread();

        ITask* set_task(ITask*task);
        bool   detachable() const { return m_detachable; }
        bool   set_detachable(bool enable);

        bool   start();
        int    join();
        bool   is_started() const { return m_threadStarted; }

        ITask* get_task() const { return m_task; }
        
    protected:
        static void* run(void*arg);        

    private:
        pthread_t m_tid;
        pthread_attr_t m_attr;
        
        ITask* m_task;
        bool m_threadStarted;
        bool m_detachable;
};



#endif

