#include "HttpServer.h"

#include "net/SocketServer.h"

#include "sys/Log.h"
#include "thread/ITask.h"
#include "thread/Thread.h"
#include "thread/ThreadPool.h"
#include "misc/LockFreeBuffer.h"
#include "misc/LockFreeList.h"

#include "net/http/HttpBuffer.h"
#include "net/http/HttpContext.h"

#include "pthread.h"
#include "assert.h"

#include <functional>


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
        bool PostSockMsg(ConnMessage* msg);

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

class HttpImpl:public noncopyable
{
    public:

        HttpImpl(HttpServer* host, const char* addr, int port = 80);
        ~HttpImpl();

        void StartServer();
        void StopServer();

        void ReleaseSockMsg(ConnMessage* msg);

        void SendData(int connid, const char* data, int sz, bool copy = true);
        void CloseConnection(int connid);

        static void SocketEventHandler(SocketEvent evt);

    private:

        std::string addr_;
        int port_;
        SocketServer tcpServer_;

        ThreadPool threadPool_;
        LockFreeBuffer bufferPool_;
        LockFreeBuffer msgPool_;

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

bool HttpTask::PostSockMsg(ConnMessage* msg)
{
    return sockMsgQueue_.Push(msg);
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
        case SC_ACCEPT:
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

//-----------------------HTTP-IMPLE------------------------
//
HttpImpl::HttpImpl(HttpServer* host, const char* addr, int port)
    :addr_(addr)
    ,port_(port)
    ,tcpServer_()
    ,threadPool_(3)
    ,bufferPool_(4096, 512)
    ,msgPool_(4096, sizeof(ConnMessage))
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

void HttpImpl::StartServer()
{
    assert(threadPool_.StartPooling());

    tcpServer_.SetWatchAcceptedSock(true);
    tcpServer_.RegisterSocketEventHandler(&HttpImpl::SocketEventHandler);
    tcpServer_.StartServer(addr_.c_str(), port_, (uintptr_t)this);
}

void HttpImpl::StopServer()
{
    tcpServer_.StopServer((uintptr_t)this);
    // then wait until server stop.
    // handle it in SocketEventHandler
}

void HttpImpl::CloseConnection(int connid)
{
    conn_[connid]->ClearMsgQueue();
    tcpServer_.CloseSocket(connid, (uintptr_t)this);
}

void HttpImpl::SendData(int connid, const char* data, int sz, bool copy)
{
    tcpServer_.SendBuffer(connid, data, sz, copy);
}


void HttpImpl::ReleaseSockMsg(ConnMessage* msg)
{
    msgPool_.ReleaseBuffer(msg);
}


void HttpImpl::SocketEventHandler(SocketEvent evt)
{
    HttpImpl* impl = (HttpImpl*)evt.msg.opaque;

    assert(impl);

    switch (evt.code)
    {
        case SC_EXIT:
            {
                impl->threadPool_.StopPooling();
            }
            break;
        case SC_CLOSE:
            {
                impl->conn_[evt.msg.id]->ReleaseTask();
            }
            break;
        case SC_ACCEPT:
        case SC_SEND:
        case SC_DATA:
            {
                ConnMessage* conn_msg = (ConnMessage*)impl->msgPool_.AllocBuffer();
                if (conn_msg == NULL)
                {
                    SocketServer::DefaultSockEventHandler(evt);
                    slog(LOG_ERROR, "httpserver error, out of msg buffer, dropping package");
                    return;
                }

                int id = evt.msg.id;
                if (evt.code == SC_ACCEPT)
                {
                    id = evt.msg.ud;
                    evt.msg.id = id;
                    impl->conn_[id]->ResetTask(id);
                }

                conn_msg->code = evt.code;
                conn_msg->msg  = evt.msg;

                if (!impl->conn_[id]->PostSockMsg(conn_msg)
                        || !impl->threadPool_.PostTask(impl->conn_[id]))
                {
                    SocketServer::DefaultSockEventHandler(evt);
                    slog(LOG_ERROR, "connection(%d) msg queue full, drop package", id);
                    return;
                }
            }
            break;
        case SC_ERROR:
        case SC_BADSOCK:
            {
                slog(LOG_ERROR, "socket error, connect id(%d)\n", evt.msg.id);
            }
            break;
        default:
            {
                SocketServer::DefaultSockEventHandler(evt);
            }
            break;
    }
}

///////////////////// HTTP SERVER /////////////////////////////

HttpServer::HttpServer(const char* addr, int port)
    :impl_(new HttpImpl(this, addr, port))
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

void HttpServer::StartServer()
{
    impl_->StartServer();
}

void HttpServer::StopServer()
{
    impl_->StopServer();
}

