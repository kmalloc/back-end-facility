#ifndef __HTTP_WRITER_H__
#define __HTTP_WRITER_H__

#include "misc/NonCopyable.h"

#include "net/SocketBuffer.h"
#include "net/http/HttpBuffer.h"
#include "net/http/HttpConnection.h"
#include "net/http/HttpRequest.h"
#include "net/http/HttpResponse.h"

class HttpReader: public noncopyable
{
    public:

        HttpReader(HttpConnection& conn);
        ~HttpReader();

        void CleanData();
        void ResetReader();
        int  ProcessHttpRead();
        bool IsKeepAlive() const { return keepalive_; }
        bool ShouldResponse() const { return curStage_ == HS_RESPONSE; }
        void SetFinishRead() { FinishResponse(); }

        HttpRequest& GetHttpRequest() { return request_; }

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

        bool ShouldParseRequestLine() const { return curStage_ == HS_REQUEST_LINE; }
        bool ShouldParseHeader() const { return curStage_ == HS_HEADER; }
        bool ShouldParseBody() const { return curStage_ == HS_BODY; }

        void FinishParsingRequestLine() { curStage_ = HS_HEADER; }
        void FinishParsingHeader() { curStage_ = HS_BODY; }
        void FinishParsingBody() { curStage_ = HS_RESPONSE; }
        void FinishResponse() { curStage_ = HS_INVALID; }

        bool ParseRequestLine();
        bool ParseHeader();
        bool ParseBody();

    private:

        bool keepalive_;
        ParseStage curStage_;
        HttpRequest request_;

        HttpReadBuffer buffer_;
        HttpConnection& conn_;
};

class HttpWriter: public noncopyable
{
    public:

        HttpWriter(HttpConnection& conn);
        ~HttpWriter();

        void ResetWriter();
        SocketBufferNode* AllocWriteBuffer(int sz);
        void AddToWriteList(SocketBufferNode* node);
        int  SendPendingBuffer();

    private:

        HttpConnection& conn_;
        SocketBufferList pendingWrite_;
        HttpWriteBuffer buffer_;
};

#endif

