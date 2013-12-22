#include "HttpServer.h"

#include "net/SocketServer.h"

#include "thread/ITask.h"
#include "thread/Thread.h"
#include "thread/ThreadPool.h"
#include "misc/LockFreeBuffer.h"
#include "misc/LockFreeList.h"

#include "net/http/HttpBuffer.h"
#include "net/http/HttpContext.h"

#include "pthread.h"
#include "assert.h"


struct ConnMessage
{
    int code;
    SocketMessage msg;
};

static void DefaultHttpRequestHandler(const HttpRequest&, HttpResponse& response)
{
    response.SetCloseConn(true);
}

// bind each connection to one thread.
class HttpTask: public ITask
{
    public:

        HttpTask(HttpServer* server, LockFreeBuffer& alloc);
        ~HttpTask();

        void ResetTask(int connid);
        void PostSockMsg(SocketMessage* msg);

        // release only when connectioin is close
        void ReleaseTask();

        // initial on socket connected.
        void InitConnection();

        virtual void Run();

        void ClearMsgQueue();

    private:

        void ProcessHttpData(const char* data, size_t sz);
        void OnConnectionClose();
        void ProcessSocketMessage(ConnMessage* msg);

        HttpServer* httpServer_;
        HttpContext context_;

        // one http connection can only be handled in one thread
        // so need to queue the msg
        LockFreeListQueue sockMsgQueue_;
};

class HttpImpl:public ThreadBase
{
    public:

        HttpImpl(HttpServer* host, const char* addr);
        ~HttpImpl();

        void StartServer();
        void StopServer();

        void ReleaseSockMsg(ConnMessage* msg);

        void SendData(int connid, const char* data, int sz, bool copy = true);
        void CloseConnection(int connid);

    protected:

        virtual void Run();

        bool ParseRequestLine(HttpBuffer& buffer) const;
        bool ParseHeader(HttpBuffer& buffer, std::map<std::string, std::string>& output) const;
        std::string ParseBody(HttpBuffer& buffer) const;

    private:

        std::string addr_;
        SocketServer tcpServer_;

        ThreadPool threadPool_;
        LockFreeBuffer bufferPool_;

        // connection id to index into it
        HttpTask** conn_;
};

HttpTask::HttpTask(HttpServer* server, LockFreeBuffer& alloc)
    : ITask(false)
    , httpServer_(server)
    , context_(*server, alloc, &DefaultHttpRequestHandler)
    , sockMsgQueue_(64)
{
}

HttpTask::~HttpTask()
{
    ReleaseTask();
}

void HttpTask::ReleaseTask()
{
    context_.ReleaseContext();
    ClearMsgQueue();
}

void HttpTask::OnConnectionClose()
{
    context_.ReleaseContext();
}

void HttpTask::ClearMsgQueue()
{
    ConnMessage* msg;
    while (sockMsgQueue_.Pop((void**)&msg))
    {
        httpServer_->GetImpl()->ReleaseSockMsg(msg);
    }
}

void HttpTask::ResetTask(int connid)
{
    context_.ResetContext(connid);
}

void HttpTask::ProcessHttpData(const char* data, size_t sz)
{
    context_.AppendData(data, sz);
    context_.ProcessHttpRequest();
}

void HttpTask::ProcessSocketMessage(ConnMessage* msg)
{
    switch (msg->code)
    {
        case SC_DATA:
            {
                ProcessHttpData(msg->msg.data, msg->msg.ud);
            }
            break;
        case SC_CONNECTED:
            {
                // connection established, bind connection to thread.
                int pid = pthread_self();
                ITask::SetAffinity(pid);
            }
            break;
        case SC_SEND: // send is asynchronous, only when received this msg, can we try to close the http connection if necessary.
            {
                context_.HandleSendDone();
            }
            break;
        default:
            {
                // not interested in other events.
                // make sure http server will not throw them here.
                assert(0);
            }
    }
}

void HttpTask::Run()
{
    ConnMessage* msg;
    while (sockMsgQueue_.Pop((void**)&msg))
    {
        ProcessSocketMessage(msg);
        httpServer_->GetImpl()->ReleaseSockMsg(msg);
    }
}

HttpImpl::HttpImpl(HttpServer* host, const char* addr)
    :addr_(addr)
    ,tcpServer_()
    ,threadPool_(3)
    ,bufferPool_(2048, 512)
{
    int i = 0;

    try
    {
        conn_ = new HttpTask*[SocketServer::max_conn_id];

        while (i < SocketServer::max_conn_id)
        {
            conn_[i] = new HttpTask(host, bufferPool_);
            ++i;
        }
    }
    catch (...)
    {
        for (int j = 0; j < i; ++j)
        {
            delete conn_[j];
        }

        delete[] conn_;

        throw "out of memory";
    }
}

HttpImpl::~HttpImpl()
{
    for (int i = 0; i < SocketServer::max_conn_id; ++i)
        delete conn_[i];

    delete[] conn_;
}

void HttpImpl::CloseConnection(int connid)
{
    conn_[connid]->ClearMsgQueue();
    tcpServer_.CloseSocket(connid);
}

void HttpImpl::SendData(int connid, const char* data, int sz, bool copy)
{
    tcpServer_.SendBuffer(connid, data, sz, copy);
}


void HttpImpl::ReleaseSockMsg(ConnMessage* msg)
{
    bufferPool_.ReleaseBuffer(msg);
}


void HttpImpl::Run()
{
    // TODO
}

///////////////////// HTTP SERVER /////////////////////////////



HttpServer::HttpServer(const char* addr)
    :impl_(new HttpImpl(this, addr))
{
}

HttpServer::~HttpServer()
{
    delete impl_;
}


void HttpServer::SendData(int connid, const char* data, int sz, bool copy)
{
    impl_->SendData(connid, data, sz, copy);
}

void HttpServer::CloseConnection(int connid)
{
    impl_->CloseConnection(connid);
}

