#include "HttpContext.h"
#include "sys/Log.h"

#include <stdlib.h>
#include <string>
#include <algorithm>

HttpContext::HttpContext(SocketServer& server, HttpCallBack cb)
    : keepalive_(false), status_(HS_CLOSED), curStage_(HS_INVALID)
    , buffer_(), conn_(server), callBack_(cb)
{
}

HttpContext::~HttpContext()
{
    ReleaseContext();
}

void HttpContext::ResetContext(int connid)
{
    ReleaseContext();

    status_ = HS_CONNECTED;
    buffer_.ReleaseBuffer();
    conn_.ResetConnection(connid);
}

// call this function only when connection is already closed
void HttpContext::ReleaseContext()
{
    status_ = HS_CLOSED;
    keepalive_ = false;
    curStage_ = HS_REQUEST_LINE;

    request_.CleanUp();
    response_.CleanUp();

    conn_.ReleaseConnection();
    buffer_.ReleaseBuffer();
}

// in normal case, this should be called after respone is done.
void HttpContext::CleanData()
{
    request_.CleanUp();
    response_.CleanUp();
    curStage_ = HS_INVALID;
}

void HttpContext::ForceCloseConnection()
{
    status_ = HS_CLOSING;
    conn_.CloseConnection();
}

void HttpContext::DoResponse()
{
    callBack_(request_, response_);
    if (response_.ShouldCloseConnection())
    {
        keepalive_ = false;
    }

    if (response_.GetShouldResponse())
    {
        size_t sz = response_.GetResponseSize() + 8;
        char* buf = (char*)malloc(sz);
        if (buf)
        {
             sz = response_.GenerateResponse(buf, sz);
             conn_.SendData(buf, sz, false);
        }
    }

    FinishResponse();
    CleanData();
}

void HttpContext::HandleSendDone()
{
    if (!keepalive_)
    {
       // ForceCloseConnection();
    }
}

void HttpContext::ProcessHttpRequest()
{
    if (status_ != HS_CONNECTED) return;

    HttpBufferEntity entity;
    char* buf = buffer_.GetReadBuffer(entity);

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
            request_.SetUrlData(data);
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

    static const char* ctrl= HttpBuffer::CTRL;
    static const size_t ctrl_len = strlen(ctrl);
    static const char* end_of_ctrl = ctrl + ctrl_len;

    static const char* header_delim = HttpBuffer::HEADER_DELIM;
    static const size_t header_delim_len = strlen(header_delim);
    static const char* end_of_header_delim = header_delim + header_delim_len;

    while (1)
    {
        const char* line_end = std::search(start, end, ctrl, end_of_ctrl);
        if (line_end == end) return true;

        const char* colon = std::search(start, line_end, header_delim, end_of_header_delim);
        if (colon == line_end)
        {
            // end of headers.
            buffer_.Consume(line_end - start + ctrl_len);
            break;
        }

        std::string key(start, colon);
        std::string value(colon + 2, line_end);

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
   const char* start = buffer_.GetStart();
   const char* end   = buffer_.GetEnd();

   if (request_.GetHttpMethod() != HttpRequest::HM_POST)
   {
       buffer_.Consume(end - start);
       FinishParsingBody();
       return true;
   }

   size_t contentLen = request_.GetBodyLength();
   if (contentLen == 0)
   {
       std::string len = request_.GetHeaderValue("Content-Length");
       if (len.empty()) return true; // no body

       contentLen = atoi(len.c_str());

       if (contentLen == 0) return true;
       else if (contentLen >= HttpRequest::MaxBodyLength) return false;

       request_.SetBodySize(contentLen);
   }

   size_t len_of_data_received = request_.GetCurBodyLength();
   if (len_of_data_received >= contentLen) return true;

   size_t left = contentLen - len_of_data_received;

   if (size_t(end - start) > left) return false;
   else if (size_t(end - start) == left) FinishParsingBody();

   request_.AppendBody(start, end);

   buffer_.Consume(end - start);

   return true;
}

