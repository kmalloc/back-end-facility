#ifndef __HTTP_CONTEXT_H__
#define __HTTP_CONTEXT_H__

#include "misc/NonCopyable.h"
#include "misc/LockFreeBuffer.h"
#include "net/http/HttpBuffer.h"

class HttpContext: public NonCopyable
{
    public:

        HttpContext(LockFreeBuffer& alloc);
        ~HttpContext();

        void ResetContext(bool keepalive = false);
        void ReleaseContext();

        bool ShouldParseRequestLine() const { return curStage_ == HS_REQUEST_LINE; }
        bool ShouldParseHeader() const { return curStage_ == HS_HEADER; }
        bool ShouldParseBody() const { return curStage_ == HS_BODY; }

        void FinishParsingRequestLine() { curStage_ = HS_HEADER; }
        void FinishParsingHeader() { curStage_ = HS_BODY; }
        void FinishParsingBody(() { curStage_ = HS_INVALID; }

        enum
        {
            HS_REQUEST_LINE,
            HS_HEADER,
            HS_BODY,
            HS_INVALID
        };

    private:

        bool keepalive_;
        int curStage_;

        HttpBuffer buffer_;
};

#endif

