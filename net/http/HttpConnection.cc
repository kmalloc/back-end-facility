#include "HttpConnection.h"


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

    sockServer_.CloseSocket(connId_);
    connId_ = -1;
}

void HttpConnection::ResetConnection(int id)
{
    connId_ = id;
}

