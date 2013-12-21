#ifndef __HTTP_SERVER_H__
#define __HTTP_SERVER_H__

#include "misc/NonCopyable.h"

class HttpImpl;

class HttpServer: public noncopyable
{
    public:

        HttpServer(const char* addr);
        ~HttpServer();

        void StartServer();
        void StopServer();

        void SendData(int connid, const char* data, int sz, bool copy = true);
        void CloseConnection(int connid);

        HttpImpl* GetImpl() {  return impl_; }

    private:

        HttpImpl* impl_;
};

#endif

