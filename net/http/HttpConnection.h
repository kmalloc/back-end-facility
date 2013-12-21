#ifndef __HTTP_CONNECTION_H__
#define __HTTP_CONNECTION_H__

#include <stdlib.h>
#include <assert.h>
#include "misc/NonCopyable.h"
#include "net/http/HttpServer.h"

class HttpConnection: public noncopyable
{
    public:

        HttpConnection(HttpServer& server);

        void ResetConnection(int connid);

        void SendData(const char* data, size_t sz, bool copy = true);
        void CloseConnection();

        int GetConnectionId() const { return connId_; }

    private:

        int connId_;
        HttpServer& sockServer_;
};

#endif

