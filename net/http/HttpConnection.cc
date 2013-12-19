#include "HttpConnection.h"


HttpConnection::HttpConnection(SocketServer& server, int id)
    :sockServer_(server), connId_(id)
{
}

void HttpConnection::SendData(const char* data, size_t sz, bool copy)
{
    assert(connId_ != -1);
    sockServer_.SendBuffer(connId_, data, sz, copy);
}

void HttpConnection::CloseConnection()
{
    sockServer_.CloseSocket(connId_);
    connId_ = -1;
}

void HttpConnection::ResetConnection(SocketServer& server, int id)
{
    sockServer_ = server;
    connId_ = id;
}

