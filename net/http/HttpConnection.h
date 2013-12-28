#ifndef __HTTP_CONNECTION_H__
#define __HTTP_CONNECTION_H__

#include <stdlib.h>
#include <assert.h>
#include "misc/NonCopyable.h"
#include "net/SocketServer.h"

class HttpConnection: public noncopyable
{
    public:

        HttpConnection(SocketServer& server);

        void ResetConnection(int connid);

        void SendData(const char* data, size_t sz, bool copy = true);

        void CloseConnection();
        void WatchConnection(uintptr_t opaque, short off);
        void ReleaseConnection();

        int GetConnectionId() const { return connId_; }

    private:

        int connId_;
        SocketServer& sockServer_;
};

#endif

