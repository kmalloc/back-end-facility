#include "HttpServer.h"

#include "net/SocketServer.h"

#include "thread/ITask.h"
#include "thread/Thread.h"
#include "misc/LockFreeBuffer.h"

class HttpBuffer:public NonCopyable
{
    public:

        HttpBuffer();
        ~HttpBuffer();

        void  Consume(int sz);
        void  Append(const char* data);
        char* Find(char c) const;
        char* Find(const char* str) const;

    private:

        char* buff_; // allocate from lockfree buffer pool
        int start_;
        int end_;
        int cur_;
};

class HttpContext: public NonCopyable
{
    public:

        HttpContext();
        ~HttpContext();

        enum
        {
            HS_REQUEST_LINE,
            HS_HEADER,
            HS_BODY,
            HS_INVALID
        };

    private:

        bool keepalive_;
        char* buff_;
        int curStage_;
};

class HttpTask: public ITask
{
    public:

        HttpTask();
        ~HttpTask();

        virtual void Run();

    private:

        HttpContext context_;
};

class HttpImpl:public ThreadBase
{
    public:

        HttpImpl(const char* addr);
        ~HttpImpl();

        void StartServer();
        void StopServer();

    protected:

        virtual void Run();

        bool ParseRequestLine(HttpBuffer& buffer);
        bool ParseHeader(HttpBuffer& buffer, map<string, string>& output);
        string ParseBody(HttpBuffer& buffer);

    private:

        SocketServer tcpServer_;

        // connection id to index into it
        HttpTask conn_[];
};

