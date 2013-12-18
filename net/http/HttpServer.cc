#include "HttpServer.h"

#include "net/SocketServer.h"

#include "thread/ITask.h"
#include "thread/Thread.h"
#include "thread/ThreadPool.h"
#include "misc/LockFreeBuffer.h"
#include "misc/LockFreeListQueue.h"

#include "net/http/HttpBuffer.h"
#include "net/http/HttpContext.h"

struct ConnMessage
{
    int code;
    SocketMessage msg;
};

class HttpTask: public ITask
{
    public:

        HttpTask(HttpImpl* server, LockFreeBuffer& alloc);
        ~HttpTask();

        void ResetTask();
        void PostSockMsg(SocketMessage* msg);

        // release only when connectioin is close
        void ReleaseTask();

        // initial on socket connected.
        void InitConnection();

        void ClearMsgQueueWithLockHold();

        virtual void Run();

    private:

        pthread_mutex_t contextLock_;

        HttpImpl* httpServer_;
        HttpContext context_;

        // one http connection can only be handled in one thread
        // so need to queue the msg
        LockFreeListQueue sockMsgQueue_;
};

HttpTask::HttpTask(HttpImpl* server, LockFreeBuffer& alloc)
    : Itask(false)
    , httpServer_(server)
    , context_(alloc)
    , sockMsgQueue_(64)
{
    pthread_mutex_init(&contextLock_, NULL);
}

HttpTask::~HttpTask()
{
    ReleaseTask();
    pthread_mutex_destroy(&contextLock_);
}

void HttpTask::ReleaseTask()
{
    pthread_mutex_lock(&contextLock_);

    context_.ReleaseContext();
    ClearMsgQueueWithLockHold();

    pthread_mutex_unlock(&contextLock_);
}

void HttpTask::ClearMsgQueueWithLockHold()
{
    ConnMessage* msg;
    while (sockMsgQueue_.Pop(&(void*)msg))
    {
        SocketServer::DefaultSockEventHandler(msg->code, msg->msg);
        httpServer_->ReleaseSockMsg(msg);
    }
}

void HttpTask::InitConnection()
{
    pthread_mutex_lock(&contextLock_);
    context_.ResetContext();
    pthread_mutex_unlock(&contextLock_);
}

class HttpImpl:public ThreadBase
{
    public:

        HttpImpl(const char* addr);
        ~HttpImpl();

        void StartServer();
        void StopServer();

    protected:

        virtual void Run();

        bool ParseRequestLine(HttpBuffer& buffer) const;
        bool ParseHeader(HttpBuffer& buffer, map<string, string>& output) const;
        string ParseBody(HttpBuffer& buffer) const;

    private:

        SocketServer tcpServer_;

        LockFreeBuffer bufferPool_;
        ThreadPool threadPool_;

        // connection id to index into it
        HttpTask conn_[];
};




