#include "HttpConnection.h"

#include "sys/Log.h"

HttpConnection::HttpConnection(SocketServer& server)
    : connId_(-1), sockServer_(server)
{
}

void HttpConnection::SendData(const char* data, size_t sz, bool copy)
{
    assert(connId_ != -1);
    sockServer_.SendBuffer(connId_, data, sz, copy);
}

void HttpConnection::CloseConnection()
{
    if (connId_ == -1) return;

    slog(LOG_VERB, "http connection close, (%d)", connId_);

    sockServer_.CloseSocket(connId_);
    connId_ = -1;
}

void HttpConnection::ReleaseConnection()
{
    connId_ = -1;
}

void HttpConnection::WatchConnection(uintptr_t opaque, short off)
{
    assert(connId_ >= 0);
    sockServer_.WatchSocket(connId_, opaque, off);
}

void HttpConnection::ResetConnection(int id)
{
    connId_ = id;
}

