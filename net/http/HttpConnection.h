#ifndef __HTTP_CONNECTION_H__
#define __HTTP_CONNECTION_H__

#include <stdlib.h>
#include <assert.h>
#include "net/SocketServer.h"
#include "misc/NonCopyable.h"

class HttpConnection: public noncopyable 
{
    public:

        HttpConnection(SocketServer& server);

        void ResetConnection(int connid);

        void SendData(const char* data, size_t sz, bool copy = true);
        void CloseConnection();

    private:

        int connId_;
        SocketServer& sockServer_;
};

#endif

