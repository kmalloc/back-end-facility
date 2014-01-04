#ifndef __HTTP_CLIENT_H__
#define __HTTP_CLIENT_H__

#include "HttpBuffer.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

#include "SocketServer.h"
#include "misc/NonCopyable.h"

class HttpClient: public noncopyable
{
    public:

        typedef void (* HttpHandler)(const HttpRequest&, HttpResponse&);

        HttpClient(HttpHandler handler = NULL);
        ~HttpClient();

        void ResetClient(SocketConnection* conn);
        void SetConnection(SocketConnection* conn);

        // return value < 0 indicate fatal error, need to close connection.
        int ProcessEvent(SocketEvent evt);

        void RegisterHttpHandler(HttpHandler handler);

    private:

        typedef int (HttpClient::*EventHandler)(SocketEvent);

        int ProcessRequestLine(SocketEvent);
        int ProcessHeader(SocketEvent);
        int ProcessBody(SocketEvent);
        int GenerateResponse(SocketEvent);
        int SendResponse(SocketEvent);

        int ParseRequestLine();
        int ParseHeader();
        int ParseBody();

        inline void FinishParsingRequestLine();
        inline void FinishParsingHeader();
        inline void FinishParsingBody();
        inline void FinishGenerateResponse();
        inline void FinishSendResponse();

        int ReadHttpData();
        void CloseConnection();

    private:

        bool keepalive_;
        HttpReadBuffer readBuffer_;
        HttpWriteBuffer writeBuffer_;

        HttpBufferList pendingWrite_;

        HttpRequest request_;
        HttpResponse response_;

        EventHandler evtHandler_;
        HttpHandler cgi_;
        SocketConnection* conn_;
};

#endif

