#ifndef _THREAD_BASE_H_
#define _THREAD_BASE_H_

#include "ITask.h"
#include "misc/noncopyable.h"
#include <pthread.h>


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

        ITask* SetTask(ITask*task) { ITask* tmp = m_task; m_task = task; return tmp;}
        const ITask* GetTask() const { return m_task; }
        
    protected:
        
        static void* RunTask(void* arg);        

        ITask* m_task;

    private:

        pthread_t m_tid;
        
        volatile bool m_busy;
        volatile bool m_detachable;
};

class ThreadBase: public Thread 
{
    public:

        ThreadBase(bool detachable = false);
        ~ThreadBase();

    protected:

        virtual void Run() = 0;

    private:

        using Thread::SetTask;
        using Thread::GetTask;

        class ThreadBaseTask: public ITask
        {
            public:

                ThreadBaseTask(ThreadBase* host) :m_host(host) {}

                virtual void Run()
                {
                    m_host->Run();
                }

            private:

                ThreadBase* m_host;
        };


        ThreadBaseTask m_task;
};

#endif

