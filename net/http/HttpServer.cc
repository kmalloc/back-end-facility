#include "HttpServer.h"

#include "sys/Log.h"
#include "thread/ITask.h"
#include "thread/Thread.h"
#include "thread/ThreadPool.h"

#include "misc/functor.h"
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

        HttpImpl(const char* addr, int port = 80);
        ~HttpImpl();

        void StartServer();
        void StopServer();

        void ReleaseSockMsg(SocketEvent* msg);

        void SocketEventHandler(SocketEvent evt);

    private:

        std::string addr_;
        int port_;
        SocketServer tcpServer_;

        ThreadPool threadPool_;
        LockFreeBuffer msgPool_;

        // connection id to index into it
        HttpTask** tasks_;
};

//-----------------------HTTP-IMPLE------------------------
//
HttpImpl::HttpImpl(const char* addr, int port)
    :addr_(addr)
    ,port_(port)
    ,tcpServer_()
    ,threadPool_()
    ,msgPool_(4096, sizeof(SocketEvent))
{
    int i = 0;

    try
    {
        tasks_ = new HttpTask*[SocketServer::max_conn_id];

        while (i < SocketServer::max_conn_id)
        {
            tasks_[i] = new HttpTask(&tcpServer_, msgPool_, i);
            ++i;
        }
    }
    catch (...)
    {
        for (int j = 0; j < i; ++j)
        {
            delete tasks_[j];
        }

        delete[] tasks_;

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

    // tcpServer_.SetWatchAcceptedSock(true);
    tcpServer_.RegisterSocketEventHandler(misc::bind(&HttpImpl::SocketEventHandler, this));
    tcpServer_.StartServer(addr_.c_str(), port_, (uintptr_t)this);
}

void HttpImpl::StopServer()
{
    tcpServer_.StopServer((uintptr_t)this);
    // then wait until server stop.
    // handle it in SocketEventHandler
}

void HttpImpl::ReleaseSockMsg(SocketEvent* msg)
{
    msgPool_.ReleaseBuffer(msg);
}

void HttpImpl::SocketEventHandler(SocketEvent evt)
{
    switch (evt.code)
    {
        case SC_EXIT:
            {
                threadPool_.StopPooling();
            }
            break;
        case SC_CLOSE:
        case SC_ACCEPTED:
        case SC_WRITEREADY:
        case SC_READREADY:
            {
                SocketEvent* conn_msg = (SocketEvent*)msgPool_.AllocBuffer();
                if (conn_msg == NULL)
                {
                    slog(LOG_ERROR, "httpserver error, out of msg buffer, dropping package");
                    return;
                }

                int id = evt.msg.fd;
                if (SC_ACCEPTED == evt.code)
                {
                    id = evt.msg.ud;
                }

                conn_msg->code = evt.code;
                conn_msg->msg  = evt.msg;

                if (!tasks_[id]->PostSockMsg(conn_msg)
                        || !threadPool_.PostTask(tasks_[id]))
                {
                    ReleaseSockMsg(conn_msg);
                    slog(LOG_ERROR, "connection(%d) msg queue full, drop package", id);
                    return;
                }
            }
            break;
        case SC_ERROR:
            {
                slog(LOG_WARN, "socket error, connect id(%d)\n", evt.msg.fd);
            }
            break;
        default:
            {
            }
            break;
    }
}

///////////////////// HTTP SERVER /////////////////////////////

HttpServer::HttpServer(const char* addr, int port)
    :impl_(new HttpImpl(addr, port))
{
}

HttpServer::~HttpServer()
{
    delete impl_;
}

void HttpServer::StartServer()
{
    impl_->StartServer();
}

void HttpServer::StopServer()
{
    impl_->StopServer();
}

