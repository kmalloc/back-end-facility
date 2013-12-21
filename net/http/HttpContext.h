#ifndef __HTTP_CONTEXT_H__
#define __HTTP_CONTEXT_H__

#include <map>
#include <stdlib.h>

#include "misc/NonCopyable.h"
#include "misc/LockFreeBuffer.h"

#include "net/SocketServer.h"
#include "net/http/HttpBuffer.h"
#include "net/http/HttpConnection.h"
#include "net/http/HttpRequest.h"
#include "net/http/HttpResponse.h"
#include "net/http/HttpCallBack.h"

class HttpContext: public noncopyable
{
    public:

        HttpContext(SocketServer& server, LockFreeBuffer& alloc, HttpCallBack cb);
        ~HttpContext();

        void ResetContext(int connid);
        void ReleaseContext();

        void AppendData(const char* data, size_t sz);

        void RunParser();

        void CleanUp();

        const HttpRequest& GetRequest() const { return request_; }

    private:

        enum
        {
            HS_REQUEST_LINE,
            HS_HEADER,
            HS_BODY,
            HS_RESPONSE,
            HS_INVALID
        };

        bool ShouldParseRequestLine() const { return curStage_ == HS_REQUEST_LINE; }
        bool ShouldParseHeader() const { return curStage_ == HS_HEADER; }
        bool ShouldParseBody() const { return curStage_ == HS_BODY; }
        bool ShouldResponse() const { return  curStage_ == HS_RESPONSE; }

        void FinishParsingRequestLine() { curStage_ = HS_HEADER; }
        void FinishParsingHeader() { curStage_ = HS_BODY; }
        void FinishParsingBody() { curStage_ = HS_RESPONSE; }
        void FinishResponse() { curStage_ = HS_INVALID; }

        bool ParseRequestLine();
        bool ParseHeader();
        bool ParseBody();
        void DoResponse();

    private:

        void ForceCloseConnection();

        bool keepalive_;
        int curStage_;

        HttpRequest request_;
        HttpResponse response_;
        HttpBuffer buffer_;
        HttpConnection conn_;

        // call cgi program
        const HttpCallBack callBack_;
};

#endif

