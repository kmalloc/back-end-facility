#include "HttpConnection.h"


HttpConnection::HttpConnection(HttpServer& server)
    : connId_(-1), sockServer_(server)
{
}

void HttpConnection::SendData(const char* data, size_t sz, bool copy)
{
    assert(connId_ != -1);
    sockServer_.SendData(connId_, data, sz, copy);
}

void HttpConnection::CloseConnection()
{
    if (connId_ == -1) return;

    sockServer_.CloseConnection(connId_);
    connId_ = -1;
}

void HttpConnection::ResetConnection(int id)
{
    connId_ = id;
}

