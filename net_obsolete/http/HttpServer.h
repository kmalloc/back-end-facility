#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include "misc/NonCopyable.h"
#include "net/SocketServer.h"

class HttpImpl;

class HttpServer: public noncopyable
{
    public:

        HttpServer(const char* addr, int port = 80);
        ~HttpServer();

        void StartServer();
        void StopServer();

    private:

        HttpImpl* impl_;
};

#endif

