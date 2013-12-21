#include "HttpContext.h"
#include "sys/Log.h"

#include <stdlib.h>
#include <string>
#include <algorithm>

HttpContext::HttpContext(SocketServer& server, LockFreeBuffer& alloc, HttpCallBack cb)
    : keepalive_(false), curStage_(HS_INVALID)
    , buffer_(alloc), conn_(server), callBack_(cb)
{
}

HttpContext::~HttpContext()
{
    ReleaseContext();
}

void HttpContext::ResetContext(int connid)
{
    curStage_ = HS_REQUEST_LINE;

    buffer_.ResetBuffer();
    conn_.ResetConnection(connid);
}

void HttpContext::ReleaseContext()
{
    request_.CleanUp();
    response_.CleanUp();

    buffer_.ReleaseBuffer();
}

void HttpContext::AppendData(const char* data, size_t sz)
{
    size_t size = buffer_.Append(data, sz);

    if (size < sz)
    {
        slog(LOG_WARN, "http request, data lost");
    }
}

// in normal case, this should be called after respone is done.
void HttpContext::CleanUp()
{
    request_.CleanUp();
    response_.CleanUp();

    if (keepalive_ == false)
    {
        conn_.CloseConnection();
    }
}

void HttpContext::ForceCloseConnection()
{
    conn_.CloseConnection();
    ReleaseContext();
}

void HttpContext::DoResponse()
{
    callBack_(request_, response_);
    if (response_.ShouldCloseConnection())
    {
        ForceCloseConnection();
    }
}

void HttpContext::RunParser()
{
    if (ShouldParseRequestLine())
    {
        if (!ParseRequestLine())
        {
            ForceCloseConnection();
            return;
        }
    }

    if (ShouldParseHeader())
    {
        if (!ParseHeader())
        {
            ForceCloseConnection();
            return;
        }
    }

    if (ShouldParseBody())
    {
        if (!ParseBody())
        {
            ForceCloseConnection();
            return;
        }
    }

    if (ShouldResponse())
    {
        DoResponse();
    }
}

bool HttpContext::ParseRequestLine()
{
    const char* start = buffer_.GetStart();
    const char* end   = buffer_.GetEnd();

    const char* delim = std::find(start, end, ' ');

    // short of data
    if (delim == end) return true;

    std::string method(start, delim);

    if (request_.SetHttpMethod(method))
    {
        buffer_.Consume(delim - start + 1);

        start = buffer_.GetStart();
        delim = std::find(start, end, ' ');

        // short of data
        if (delim == end) return true;

        const char* question_mark = std::find(start, end, '?');

        if (question_mark != end)
        {
            std::string url(start, question_mark);
            request_.SetUrl(url);

            std::string data(question_mark + 1, delim);
            request_.SetPostData(data);
        }
        else
        {
            std::string url(start, delim);
            request_.SetUrl(url);
        }

        buffer_.Consume(delim - start + 1);
        start = buffer_.GetStart();

        const char* ctrl= HttpBuffer::CTRL;
        const size_t ctrl_len = strlen(ctrl);
        const char* end_of_ctrl = ctrl + ctrl_len;

        const char* end_of_req = std::search(start, end, ctrl, end_of_ctrl);

        // short of data
        if (end_of_req == end) return true;

        // "HTTP/1.0" "HTTP/1.1"
        const char http_version[] = "HTTP/1.1";

        if (end_of_req == end || end_of_req - start != sizeof(http_version) - 1) return false;

        std::string version(start, end_of_req);

        if (!request_.SetVersion(version)) return false;

        buffer_.Consume(sizeof(http_version) - 1 + ctrl_len);

        FinishParsingRequestLine();
        return true;
    }

    return false;
}

bool HttpContext::ParseHeader()
{
    const char* start = buffer_.GetStart();
    const char* end   = buffer_.GetEnd();

    const char* ctrl= HttpBuffer::CTRL;
    const size_t ctrl_len = strlen(ctrl);
    const char* end_of_ctrl = ctrl + ctrl_len;

    while (1)
    {
        const char* line_end = std::search(start, end, ctrl, end_of_ctrl);
        if (line_end == end) return true;

        const char* colon = std::find(start, line_end, ':');
        if (colon == line_end)
        {
            // end of headers.
            buffer_.Consume(line_end - start + ctrl_len);
            break;
        }

        std::string key(start, colon);
        std::string value(colon + 1, line_end);

        request_.AddHeader(key.c_str(), value.c_str());

        buffer_.Consume(line_end - start + ctrl_len);
        start = buffer_.GetStart();
    }

    std::string connection = request_.GetHeaderValue("Connection");

    keepalive_ = (connection == "Keep-Alive");
    FinishParsingHeader();
    return true;
}


bool HttpContext::ParseBody()
{
   if (request_.GetHttpMethod() != HttpRequest::HM_POST) return false;

   size_t contentLen = request_.GetBodyLength();
   if (contentLen == 0)
   {
       std::string len = request_.GetHeaderValue("Content-Length");
       if (len.empty()) return false;

       contentLen = atoi(len.c_str());

       if (contentLen == 0) return true;
       else if (contentLen >= HttpRequest::MaxBodyLength) return false;

       request_.SetBodySize(contentLen);
   }

   size_t len_of_data_received = request_.GetCurBodyLength();
   if (len_of_data_received >= contentLen) return true;

   size_t left = contentLen - len_of_data_received;

   const char* start = buffer_.GetStart();
   const char* end   = buffer_.GetEnd();

   if (size_t(end - start) > left) return false;
   else if (size_t(end - start) == left) FinishParsingBody();

   request_.AppendBody(start, end);

   buffer_.Consume(end - start);

   return true;
}

