#ifndef __HTTP_CONNECTION_H__
#define __HTTP_CONNECTION_H__

#include "net/SocketServer.h"
#include "misc/NonCopyable.h"

class HttpConnection: public NonCopyable
{
    public:

        HttpConnection(SocketServer& server, int connid);

        void ResetConnection(SocketServer& server, int connid);

        void SendData(const char* data, size_t sz, bool copy = true);
        void CloseConnection();

    private:

        int connId_;
        SocketServer& sockServer_;
};

#endif

