#ifndef __HTTP_CONTEXT_H__
#define __HTTP_CONTEXT_H__

#include <map>
#include <stdlib.h>

#include "misc/NonCopyable.h"
#include "misc/LockFreeBuffer.h"

#include "net/http/HttpBuffer.h"
#include "net/http/HttpConnection.h"
#include "net/http/HttpRequest.h"
#include "net/http/HttpResponse.h"
#include "net/http/HttpCallBack.h"

class HttpContext: public NonCopyable
{
    public:

        HttpContext(LockFreeBuffer& alloc, HttpCallBack cb);
        ~HttpContext();

        void ResetContext(bool keepalive = false);
        void ReleaseContext();

        void AppendData(const char* data, size_t sz);

        void RunParser();

        void CleanUp();

    private:

        bool ShouldParseRequestLine() const { return curStage_ == HS_REQUEST_LINE; }
        bool ShouldParseHeader() const { return curStage_ == HS_HEADER; }
        bool ShouldParseBody() const { return curStage_ == HS_BODY; }

        void FinishParsingRequestLine() { curStage_ = HS_HEADER; }
        void FinishParsingHeader() { curStage_ = HS_BODY; }
        void FinishParsingBody() { curStage_ = HS_RESPONSE; }
        void FinishResponse() { curStage_ = HS_INVALID; }

        void ParseRequestLine();
        void ParseHeader();
        void ParseBody();
        void DoResponse();

        enum
        {
            HS_REQUEST_LINE,
            HS_HEADER,
            HS_BODY,
            HS_RESPONSE,
            HS_INVALID
        };

    private:

        bool keepalive_;
        int curStage_;

        HttpRequest request_;
        HttpResponse response_;
        HttpBuffer buffer_;
        HttpConnection conn_;

        const HttpCallBack callBack_;
};

#endif

