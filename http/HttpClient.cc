#include "HttpClient.h"

#include <algorithm>

static const char HTTP_CTRL[] = "\r\n";
static const char HTTP_HEADER_DELIM[] = ": ";

static const int HTTP_CTRL_LEN = sizeof(HTTP_CTRL) - 1;
static const int HTTP_HEADER_DELIM_LEN = sizeof(HTTP_HEADER_DELIM) - 1;

HttpClient::HttpClient(HttpHandler handler)
    :keepalive_(false)
    ,evtHandler_(&HttpClient::ProcessRequestLine)
    ,cgi_(handler)
{
}

HttpClient::~HttpClient()
{
}

void HttpClient::SetConnection(SocketConnection* conn)
{
    conn_ = conn;
}

void HttpClient::RegisterHttpHandler(HttpHandler handler)
{
    cgi_ = handler;
}

void HttpClient::ResetClient(SocketConnection* conn)
{
    keepalive_ = false;
    request_.CleanUp();
    response_.CleanUp();
    readBuffer_.ResetBuffer();
    HttpBuffer* buf = pendingWrite_.PopFront();
    while (buf)
    {
        writeBuffer_.ReleaseWriteBuffer(buf);
        buf = pendingWrite_.PopFront();
    }

    conn_ = conn;
    evtHandler_ = &HttpClient::ProcessRequestLine;
}

void HttpClient::CloseConnection()
{
    conn_->CloseConnection();
    ResetClient(NULL);
}

int HttpClient::ProcessEvent(SocketEvent evt)
{
    int ret = 0;
    int len = 0;

    do
    {
        len = (this->*evtHandler_)(evt);
        if (len < 0)
        {
            CloseConnection();
            return -1;
        }

        ret += len;

    } while (len > 0);

    return ret;
}

int HttpClient::ReadHttpData()
{
    int sz = readBuffer_.GetContenLen();
    if (sz) return 0;

    char* buf = readBuffer_.GetFreeBuffer(sz);

    if (sz) sz = conn_->ReadBuffer(buf, sz);

    if (sz > 0) readBuffer_.IncreaseContentRange(sz);

    return sz;
}

int HttpClient::ProcessRequestLine(SocketEvent evt)
{
    if (evt.code == SC_READ)
    {
        int ret = ReadHttpData();
        if (ret < 0) return ret;
    }

    return ParseRequestLine();
}

int HttpClient::ProcessHeader(SocketEvent evt)
{
    if (evt.code == SC_READ)
    {
        int ret = ReadHttpData();
        if (ret < 0) return ret;
    }

    return ParseHeader();
}

int HttpClient::ProcessBody(SocketEvent evt)
{
    if (evt.code == SC_READ)
    {
        int ret = ReadHttpData();
        if (ret < 0) return ret;
    }

    return ParseBody();
}

// TODO, use trie to improve parsing efficiency.
int HttpClient::ParseRequestLine()
{
    const char* start = readBuffer_.GetContentStart();
    const char* end   = readBuffer_.GetContentEnd();

    const char* delim = std::find(start, end, ' ');

    int len = 0;

    // short of data
    if (delim == end) return 0;

    if (request_.SetHttpMethod(start, delim))
    {
        len += delim - start + 1;
        readBuffer_.ConsumeBuffer(delim - start + 1);

        start = readBuffer_.GetContentStart();
        delim = std::find(start, end, ' ');

        // short of data
        if (delim == end) return len;

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

        len += delim - start + 1;
        readBuffer_.ConsumeBuffer(delim - start + 1);
        start = readBuffer_.GetContentStart();

        static const char* ctrl= HTTP_CTRL;
        static const size_t ctrl_len = HTTP_CTRL_LEN;
        static const char* end_of_ctrl = ctrl + ctrl_len;

        const char* end_of_req = std::search(start, end, ctrl, end_of_ctrl);

        // short of data
        if (end_of_req == end) return len;

        // "HTTP/1.0" "HTTP/1.1"
        static const char http_version[] = "HTTP/1.1";
        static const int http_version_len = sizeof(http_version) - 1;

        if (end_of_req == end || end_of_req - start != http_version_len) return -1;

        std::string version(start, end_of_req);

        if (!request_.SetVersion(version)) return -1;

        len += sizeof(http_version) - 1 + ctrl_len;
        readBuffer_.ConsumeBuffer(sizeof(http_version) - 1 + ctrl_len);

        FinishParsingRequestLine();
        return len;
    }

    return -1;
}

int HttpClient::ParseHeader()
{
    int len = 0;
    const char* start = readBuffer_.GetContentStart();
    const char* end   = readBuffer_.GetContentEnd();

    static const char* ctrl= HTTP_CTRL;
    static const size_t ctrl_len = HTTP_CTRL_LEN;
    static const char* end_of_ctrl = ctrl + ctrl_len;

    static const char* header_delim = HTTP_HEADER_DELIM;
    static const size_t header_delim_len = HTTP_HEADER_DELIM_LEN;
    static const char* end_of_header_delim = header_delim + header_delim_len;

    while (1)
    {
        const char* line_end = std::search(start, end, ctrl, end_of_ctrl);
        if (line_end == end) return true;

        const char* colon = std::search(start, line_end, header_delim, end_of_header_delim);
        if (colon == line_end)
        {
            // end of headers.
            len += line_end - start + ctrl_len;
            readBuffer_.ConsumeBuffer(line_end - start + ctrl_len);
            break;
        }

        std::string key(start, colon);
        std::string value(colon + 2, line_end);

        request_.AddHeader(key.c_str(), value.c_str());

        len += line_end - start + ctrl_len;
        readBuffer_.ConsumeBuffer(line_end - start + ctrl_len);
        start = readBuffer_.GetContentStart();
    }

    std::string connection = request_.GetHeaderValue("Connection");

    keepalive_ = (connection == "Keep-Alive");
    FinishParsingHeader();
    return len;
}

int HttpClient::ParseBody()
{
    int len = 0;
    const char* start = readBuffer_.GetContentStart();
    const char* end   = readBuffer_.GetContentEnd();

    if (request_.GetHttpMethod() != HttpRequest::HM_POST)
    {
        len += end - start;
        readBuffer_.ConsumeBuffer(end - start);
        FinishParsingBody();
        return 1;
    }

    size_t contentLen = request_.GetBodyLength();
    if (contentLen == 0)
    {
        std::string clen = request_.GetHeaderValue("Content-Length");
        if (clen.empty()) return len; // no body

        contentLen = atoi(clen.c_str());

        if (contentLen == 0) return len;
        else if (contentLen >= HttpRequest::MaxBodyLength) return -1;

        request_.SetBodySize(contentLen);
    }

    size_t len_of_data_received = request_.GetCurBodyLength();
    if (len_of_data_received >= contentLen) return len;

    size_t left = contentLen - len_of_data_received;

    if (size_t(end - start) > left) return -1;
    else if (size_t(end - start) == left) FinishParsingBody();

    request_.AppendBody(start, end);

    len += end - start;
    readBuffer_.ConsumeBuffer(end - start);

    return len;
}

int HttpClient::GenerateResponse(SocketEvent evt)
{
    // TODO
    cgi_(request_, response_);

    int sz = response_.GetResponseSize();

    HttpBuffer* buf = writeBuffer_.AllocWriteBuffer(sz);
    buf->curSize_ = response_.GenerateResponse(buf->curPtr_, buf->size_);

    pendingWrite_.PushBack(buf);
    response_.CleanUp();

    FinishGenerateResponse();
    return 1;
}

int HttpClient::SendResponse(SocketEvent evt)
{
    int len = 0;
    HttpBuffer* buf = pendingWrite_.GetFront();

    while (buf)
    {
        int sz = conn_->SendBuffer(buf->curPtr_, buf->curSize_);
        if (sz < 0) return -1;

        len += sz;
        if (sz < buf->curSize_)
        {
            buf->curPtr_ += sz;
            buf->curSize_ -= sz;
            break;
        }
        pendingWrite_.PopFront();
        writeBuffer_.ReleaseWriteBuffer(buf);
        buf = pendingWrite_.GetFront();
    }

    if (buf == NULL) FinishSendResponse();

    return len;
}

void HttpClient::FinishParsingRequestLine()
{
    evtHandler_ = &HttpClient::ProcessHeader;
}

void HttpClient::FinishParsingHeader()
{
    evtHandler_ = &HttpClient::ProcessBody;
}

void HttpClient::FinishParsingBody()
{
    evtHandler_ = &HttpClient::GenerateResponse;
}

void HttpClient::FinishGenerateResponse()
{
    evtHandler_ = &HttpClient::SendResponse;
}

void HttpClient::FinishSendResponse()
{
    evtHandler_ = &HttpClient::ProcessRequestLine;
}

