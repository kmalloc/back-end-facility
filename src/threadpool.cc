#include "threadpool.h"
#include "SpinlockQueue.h"
#include "atomic_ops.h"

#include <memory.h>


struct CompareTaskPriority 
{
    inline bool operator()(ITask*& x, ITask*&y) const 
    {
        return x->Priority() < y->Priority();
    }
};

class Dispatcher:public WorkerBodyBase
{
    public:

        Dispatcher(ThreadPool* pool, int workerNum = DEFAULT_WORKER_TASK_MSG_SIZE);
        ~Dispatcher();

        void StopRunning();
        void StopWorker();
        virtual int  GetTaskNumber();
        virtual bool HasTask();

        /*
         * worker calls this function to require task to Run
         * keep in mind that this function will be called in different threads
         */
        int SetWorkerNotify(Worker* worker);

    protected:

        virtual void HandleTask(ITask*);
        virtual ITask* GetTaskFromContainer();
        virtual bool PushTaskToContainer(ITask*);

        int  SetWorkerRequest(Worker*);
        bool HandleWorkerRequest();
        void DispatchTask(ITask*);
        Worker* SelectFreeWorker();
        
    private:

        Worker* FindRequestWorker();
        ITask*  FindRunTaskFromOtherWorker();

        int m_singleRequestThreshold;
        int m_totalRequestThreashold;

        sem_t m_freeWorker;
        const int m_workerNum;
        volatile int m_totalRequest;
        volatile int *m_request;
        ThreadPool* m_pool;

        std::vector<Worker*> m_workers;
        SpinlockQueue<ITask*, PriorityQueue<ITask*,CompareTaskPriority> > m_queue;
};


Dispatcher::Dispatcher(ThreadPool* pool, int workerNum)
    :WorkerBodyBase()
    ,m_pool(pool)
    ,m_workerNum(workerNum)
    ,m_totalRequest(0)
    ,m_singleRequestThreshold(2)
    ,m_totalRequestThreashold(m_workerNum/4 > m_singleRequestThreshold?m_workerNum/4:m_singleRequestThreshold)
{
    sem_init(&m_freeWorker,0,0);
    m_request = new int[m_workerNum];

    m_workers.reserve(m_workerNum);
    for (int i = 0; i < m_workerNum; ++i)
    {
        Worker* worker = new Worker(m_pool, i);
        worker->EnableNotify(true);
        m_workers.push_back(worker);
        m_request[i] = 0;
    }
}

Dispatcher::~Dispatcher()
{
    for (int i = 0; i < m_workerNum; ++i)
    {
        m_workers[i]->Cancel();
        delete m_workers[i];
    }

    delete []m_request;
}

void Dispatcher::StopWorker()
{
    for (int i = 0; i < m_workerNum; ++i)
    {
        m_workers[i]->StopRunning();
        m_workers[i]->Join();
    }
}

void Dispatcher::StopRunning()
{
    WorkerBodyBase::StopRunning();
    StopWorker();
}

bool Dispatcher::PushTaskToContainer(ITask* task)
{
    return m_queue.PushBack(task);
}

ITask* Dispatcher::GetTaskFromContainer()
{
    return m_queue.PopFront();
}

int Dispatcher::GetTaskNumber() 
{
    return m_queue.Size();
}

bool Dispatcher::HasTask()
{
    return !m_queue.IsEmpty();
}

/*
 * return one worker that is :
 * a) not busy.
 * b) with least working tasks
 *
 * considering the total amount of workers is usually rather small,
 * we don't have to apply any advanced algorithm and data structure to 
 * maintain the worker list.
 * just avoiding premptive operation and use brute-search is enough.
 */
Worker* Dispatcher::SelectFreeWorker()
{
    int chosen = -1, min_task = -1;

    for (int i = 0; i < m_workerNum; ++i)
    {
        int sz = m_workers[i]->GetTaskNumber(); 
        if (sz > min_task)
        {
            min_task = sz;
            i = chosen;
            if (sz == 0) break;
        }
    }

    return m_workers[chosen];
}

void Dispatcher::DispatchTask(ITask* task)
{
    Worker* worker;
    do
    {
        sem_wait(&m_freeWorker);
        worker = SelectFreeWorker();

    }while (worker == NULL);
    
    worker->PostTask(task);
}

int Dispatcher::SetWorkerNotify(Worker* worker)
{
    sem_post(&m_freeWorker);
}

int Dispatcher::SetWorkerRequest(Worker*worker)
{
    if (!HasTask())
    {
        int id = worker->GetWorkerId();
        if (id < 0 || id >= m_workerNum) return -1;

        atomic_increment(&m_request[id]);
        atomic_increment(&m_totalRequest);
        PostTask(NULL);
        return 1;
    }

    return 0;
}


/*
 * find a worker that sends most request.
 * this might fail if requst does not meet the threshhold required.
 */
Worker* Dispatcher::FindRequestWorker()
{
    int chosen, min = -1;
    for (int i = 0; i < m_workerNum; ++i)
    {
        int req = m_request[i];
        if (req > m_singleRequestThreshold && req > min)
        {
            if (m_workers[i]->IsRunning())
            {
                m_request[i] = 0;
                atomic_add(&m_totalRequest, -req);

                if (m_totalRequest < m_totalRequestThreashold)
                    return NULL;

                continue;
            }

            m_request[i] = 0;
            atomic_add(&m_totalRequest, -req);
            min = m_request[i];
            chosen = i;
            break;
        }
    }

    if (chosen != -1) return m_workers[chosen];

    m_totalRequest = m_totalRequestThreashold/2;
    return NULL;
}

ITask* Dispatcher::FindRunTaskFromOtherWorker()
{
    //TODO
    return NULL;
}

/*
 * in an ideal circumstance , this function should be called as least as possible.
 *
 */
bool Dispatcher::HandleWorkerRequest()
{
    if (HasTask())
    {
        atomic_decrement(&m_totalRequest);
        return true;
    }

    if (m_totalRequest > m_totalRequestThreashold)
    {
        Worker* worker = FindRequestWorker();
        if (worker)
        {
            ITask* task = FindRunTaskFromOtherWorker();
            if (task) 
            {
                int id = worker->GetWorkerId();
                int req = m_request[id];

                atomic_add(&m_totalRequest,-req);
                m_request[id] = 0;

                worker->PostTask(task);
            }
            else
            {
                m_totalRequest = 0;
                memset((void*)m_request, 0, sizeof(int)*m_workerNum);
            }
        }
    }
}


void Dispatcher::HandleTask(ITask* task)
{
    if (task == NULL)
    {
        //null task serves as a special msg to inform ThreadPool
        //to handle worker request.
        //request handling is troublesome, and not quite efficient, 
        //so disable it for the moment.
        //and use semaphore instead.
        //HandleWorkerRequest();
    }
    else
    {
        DispatchTask(task);
    }
}



/*
 * thread pool definition
 */

ThreadPool::ThreadPool(int num)
   :WorkerManagerBase()
{
    m_dispatcher = new Dispatcher(this, num); 
    m_worker = new Worker(m_dispatcher);
}

ThreadPool::~ThreadPool()
{
    delete m_worker;
}

bool ThreadPool::StartPooling()
{
    return m_worker->StartWorking();
}

void ThreadPool::StopPooling()
{
    m_worker->StopRunning();
}

int ThreadPool::SetWorkerNotify(Worker* worker)
{
    return m_dispatcher->SetWorkerNotify(worker);
}


