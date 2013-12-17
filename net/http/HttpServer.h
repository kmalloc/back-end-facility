#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include "misc/NonCopyable.h"

class HttpImpl;

class HttpServer: public NonCopyable
{
    public:

        HttpServer(const char* addr);
        ~HttpServer();

        void StartServer();
        void StopServer();

    private:

        HttpImpl* impl_;
};

#endif

