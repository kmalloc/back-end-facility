#include "HttpConnection.h"

#include "sys/Log.h"

HttpConnection::HttpConnection(SocketServer& server)
    : connId_(-1), sockServer_(server)
{
}

int HttpConnection::SendBuffer(const char* data, size_t sz) const
{
    assert(connId_ != -1);
    return sockServer_.SendBuffer(connId_, data, sz);
}

int HttpConnection::ReadBuffer(char* buff, int sz) const
{
    return sockServer_.ReadBuffer(connId_, buff, sz);
}

void HttpConnection::CloseConnection()
{
    if (connId_ == -1) return;

    slog(LOG_VERB, "to close http connection(%d)", connId_);

    int conn = connId_;
    connId_ = -1;
    sockServer_.CloseSocket(conn);
}

void HttpConnection::ReleaseConnection()
{
    connId_ = -1;
}

void HttpConnection::WatchConnection() const
{
    assert(connId_ >= 0);
    sockServer_.WatchSocket(connId_);
}

void HttpConnection::ResetConnection(int id)
{
    connId_ = id;
}

