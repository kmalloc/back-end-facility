#ifndef __HTTP_CONTEXT_H__
#define __HTTP_CONTEXT_H__

#include <map>
#include <algorithm>
#include <stdlib.h>

#include "misc/NonCopyable.h"
#include "misc/LockFreeBuffer.h"

#include "net/http/HttpServer.h"
#include "net/http/HttpConnection.h"
#include "net/http/HttpReaderWriter.h"
#include "net/http/HttpCallBack.h"

enum HttpStatus
{
    HS_CLOSING,
    HS_CLOSED,
    HS_CONNECTED,
};

class HttpContext: public noncopyable
{
    public:

        HttpContext(SocketServer& server, HttpCallBack cb);
        ~HttpContext();

        void ResetContext(int connid);
        void ReleaseContext();

        int ProcessHttpRead();
        int ProcessHttpWrite();

        bool IsKeepAlive() const { return httpReader_.IsKeepAlive(); }

    private:

        void DoResponse(const HttpRequest& request);
        void ForceCloseConnection();

        HttpStatus status_;
        HttpConnection conn_;
        HttpReader httpReader_;
        HttpWriter httpWriter_;

        // call cgi program
        const HttpCallBack callBack_;
};

#endif

