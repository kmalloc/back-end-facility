#ifndef __HTTP_CONTEXT_H__
#define __HTTP_CONTEXT_H__

#include <map>
#include <stdlib.h>

#include "misc/NonCopyable.h"
#include "misc/LockFreeBuffer.h"

#include "net/http/HttpServer.h"
#include "net/http/HttpBuffer.h"
#include "net/http/HttpConnection.h"
#include "net/http/HttpRequest.h"
#include "net/http/HttpResponse.h"
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

        void ProcessHttpRead();
        void ProcessHttpWrite();

        bool IsKeepAlive() const { return keepalive_; }

        enum ParseStage
        {
            HS_REQUEST_LINE,
            HS_HEADER,
            HS_BODY,
            HS_RESPONSE,
            HS_INVALID
        };

        ParseStage ParsingStage() const { return curStage_; }

    private:

        void CleanData();
        void HandleSendDone();

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

        void CleanUp();

    private:

        void ForceCloseConnection();

        bool keepalive_;
        HttpStatus status_;

        ParseStage curStage_;

        HttpRequest request_;
        HttpResponse response_;
        HttpBuffer buffer_;
        HttpConnection conn_;

        // call cgi program
        const HttpCallBack callBack_;
};

#endif

