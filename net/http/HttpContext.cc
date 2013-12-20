#include "HttpContext.h"
#include "sys/Log.h"

#include <string>

HttpContext::HttpContext(LockFreeBuffer& alloc, HttpCallBack cb)
    : keepalive_(false), curStage_(HS_INVALID)
    , buffer_(alloc), callBack_(cb)
{
}

HttpContext::~HttpContext()
{
    ReleaseContext();
}

void HttpContext::ResetContext(bool keepalive)
{
    keepalive_ = keepalive;
    curStage_ = HS_REQUEST_LINE;

    buffer_.ResetBuffer();
}

void HttpContext::ReleaseContext()
{
    request_.CleanUp();
    response_.CleanUp();

    buffer_.ReleaseBuffer();
}

void HttpContext::AppendData(const char* data, size_t sz)
{
    size_t size = buffer_.AppendData(data, sz);

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

void HttpContext::DoResponse()
{
    std::string result = callBack_(httpReqLine_);
    if (!result.empty())
    {
        conn_.SendData(result.c_str(), result.size() + 1);
    }
}

void HttpContext::RunParser()
{
    if (ShouldParseRequestLine())
    {
        if (!ParseRequestLine())
        {
            // TODO, close the connection, bad format
        }
    }

    if (ShouldParseHeader())
    {
        if (!ParseHeader())
        {
            // TODO, close the connection, bad format
        }
    }

    if (ShouldParseBody())
    {
        if (!ParseBody())
        {
            // TODO bad data.
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

        if (!buffer_.SetVersion(version)) return false;

        buffer_.Consume(sizeof(http_version) - 1 + ctrl_len);

        FinishParsingRequestLine();
        return true;
    }

    return false;
}

bool HttpContext::ParseHeader()
{

}

