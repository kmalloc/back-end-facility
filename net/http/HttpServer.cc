#include "HttpServer.h"

#include "sys/Log.h"
#include "thread/ITask.h"
#include "thread/Thread.h"
#include "thread/ThreadPool.h"
#include "misc/LockFreeBuffer.h"
#include "misc/LockFreeList.h"

#include "net/http/HttpTask.h"
#include "net/http/HttpContext.h"

#include "stdio.h"
#include "pthread.h"
#include "assert.h"


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
        HttpTask** tasks_;
};

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
        tasks_ = new HttpTask*[SocketServer::max_conn_id];

        while (i < SocketServer::max_conn_id)
        {
            tasks_[i] = new HttpTask(host, bufferPool_);
            ++i;
        }
    }
    catch (...)
    {
        for (int j = 0; j < i; ++j)
        {
            delete tasks_[j];
        }

        delete[] task_;

        throw "out of memory";
    }
}

HttpImpl::~HttpImpl()
{
    for (int i = 0; i < SocketServer::max_conn_id; ++i)
        delete tasks_[i];

    delete[] tasks_;
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
    tasks_[connid]->CloseTask();
    tcpServer_.CloseSocket(connid, (uintptr_t)this);
}

void HttpImpl::SendData(int connid, const char* data, int sz, bool copy)
{
    tcpServer_.SendBuffer(connid, data, sz, copy);
}

void HttpImpl::ReleaseSockMsg(ConnMessage* msg)
{
    SocketServer::DefaultSockEventHandler(msg->msg);
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
        case SC_ACCEPT:
            {
                int id = evt.msg.ud;
                int affinity = impl->threadPool_.PickIdleWorker();

                impl->tasks_[id]->ResetTask(id);
                impl->tasks_[id]->SetAffinity(affinity);
            }
            break;
        case SC_CLOSE:
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

                conn_msg->code = evt.code;
                conn_msg->msg  = evt.msg;

                if (!impl->tasks_[id]->PostSockMsg(conn_msg)
                        || !impl->threadPool_.PostTask(impl->tasks_[id]))
                {
                    ReleaseSockMsg(conn_msg);
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

void HttpServer::ReleaseSockMsg(ConnMessage* msg)
{
    impl_->ReleaseSockMsg(msg);
}


