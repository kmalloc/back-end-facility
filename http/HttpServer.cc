#include "HttpServer.h"

#include "sys/Log.h"

static void DefaultHttpRequestHandler(const HttpRequest& req, HttpResponse& response)
{
    response.SetShouldResponse(true);
    response.SetStatusCode(HttpResponse::HSC_200);
    response.SetStatusMessage("OK");

    using namespace std;

    const map<string, string>& headers = req.GetHeader();
    map<string, string>::const_iterator it = headers.begin();

    string body ;
    body.reserve(128);

    body = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 1.0 Frameset//EN\" \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-Frameset.dtd\">\
<html>\
<head>\
    <meta http-equiv=\"Content-Type\" content=\"text/html; charset=gb2312\" />\
    <title>miliao http server</title>\
</head>\
<body>\
 <p>";

    body += "Page auto-generated from http request header:\r\n";
    body += "</p>";
    body += "<p>";
    while (it != headers.end())
    {
        body += it->first;
        body += ":";
        body += it->second;
        body += "</p>";

        ++it;
    }

    body += "</p>";
    body += "Body from request: </p> " + req.GetHttpBody();

    body += "</p>\
             </body>\
             </html>";

    response.SetBody(body.c_str());

    char bodylen[32] = {0};
    snprintf(bodylen, 32, "%d", body.size());

    response.AddHeader("Connection", "close");
    response.AddHeader("Host", "miliao server");
    response.AddHeader("Content-Type", "text/html;charset=utf-8");
    response.AddHeader("Content-Length", bodylen);

    time_t now;
    struct tm gm;
    char tmp[64];

    now = time(NULL);
    gm = *gmtime(&now);

    strftime(tmp, sizeof(tmp), "%a, %d %b %Y %H:%S %Z", &gm);
    response.AddHeader("Date", tmp);

    response.SetCloseConn(false);
}

HttpServer::HttpServer()
    :stop_(false)
    ,watching_(false)
    ,listenFd_(-1)
    ,tcpServer_()
    ,conn_(new HttpClient*[tcpServer_.max_conn_id])
{

}

HttpServer::~HttpServer()
{
    DestroyServer();
}

void HttpServer::DestroyServer()
{
    for (int i = 0; i < tcpServer_.max_conn_id; ++i)
    {
        if (conn_[i]) delete conn_[i];
    }

    delete[] conn_;
}

void HttpServer::SetListenSock(int fd)
{
    listenFd_ = fd;
    watching_ = tcpServer_.WatchRawSocket(fd, true);
}

void HttpServer::RunServer()
{
    tcpServer_.SetWatchAcceptedSock(true);
    for (int i = 0; i < tcpServer_.max_conn_id; ++i)
    {
        conn_[i] = NULL;
    }

    RunPoll();
}

void HttpServer::PollHandler(SocketEvent evt)
{
    int id = evt.conn->GetConnectionId();
    if (conn_[id] == NULL) conn_[id] = new HttpClient(DefaultHttpRequestHandler);

    switch (evt.code)
    {
        case SC_READ:
        case SC_WRITE:
            {
                conn_[id]->ProcessEvent(evt);
            }
            break;
        case SC_ACCEPTED:
            {
                static int co = 0;
                ++co;
                slog(LOG_INFO, "accept(%d)", co);
                conn_[id]->ResetClient(evt.conn);
            }
            break;
        case SC_FAIL_CONN:
            {
                evt.conn->CloseConnection();
            }
            break;
        case SC_ERROR:
            {
            }
            break;
        default:
            {
            }
            break;
    }
}

void HttpServer::RunPoll()
{
    int num_conn;
    int total = tcpServer_.max_conn_id;

    while (stop_ == false)
    {
        SocketEvent evt;
        num_conn = tcpServer_.GetConnNumber();

        if (watching_ && num_conn > total - total/8)
        {
            watching_ = !tcpServer_.UnwatchSocket(listenFd_);
        }
        else if (!watching_ && num_conn <= total/2)
        {
            // this may be buggy, if half of the connections stay long.
            watching_ = tcpServer_.WatchRawSocket(listenFd_, true);
        }

        tcpServer_.RunPoll(&evt);
        PollHandler(evt);
    }
}

void HttpServer::SetStop()
{
    stop_ = true;
}

