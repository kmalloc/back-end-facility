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

        int SendBuffer(const char* data, size_t sz) const;
        int ReadBuffer(char* buffer, int sz) const;
        void WatchConnection() const;

        void CloseConnection();
        void ReleaseConnection();

        int GetConnectionId() const { return connId_; }

    private:

        int connId_;
        SocketServer& sockServer_;
};

#endif

