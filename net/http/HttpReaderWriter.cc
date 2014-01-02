#include "HttpReaderWriter.h"
#include "sys/Log.h"

#include <algorithm>

HttpReader::HttpReader(HttpConnection& conn)
    :conn_(conn), curStage_(HS_INVALID)
    ,keepalive_(false)
{
}

HttpReader::~HttpReader()
{
    ResetReader();
}

void HttpReader::ResetReader()
{
    keepalive_ = false;
    CleanData();
    curStage_ = HS_REQUEST_LINE;
}

// in normal case, this should be called after respone is done.
void HttpReader::CleanData()
{
    request_.CleanUp();
    curStage_ = HS_INVALID;
}

int HttpReader::ProcessHttpRead()
{
    HttpBufferEntity entity = buffer_.GetAvailableReadRange();
    int ret = 0;

    if (entity.buffer && entity.size)
    {
        ret = conn_.ReadBuffer(entity.buffer, entity.size);
        buffer_.IncreaseContentRange(ret);
        if (ret < 0) return -1;
    }

    if (ShouldParseRequestLine())
    {
        if (!ParseRequestLine()) return -2;
    }

    if (ShouldParseHeader())
    {
        if (!ParseHeader()) return -3;
    }

    if (ShouldParseBody())
    {
        if (!ParseBody()) return -4;
    }

    return ret;
}

bool HttpReader::ParseRequestLine()
{
    const char* start = buffer_.GetContentStart();
    const char* end   = buffer_.GetContentEnd();

    const char* delim = std::find(start, end, ' ');

    // short of data
    if (delim == end) return true;

    std::string method(start, delim);

    if (request_.SetHttpMethod(method))
    {
        buffer_.ConsumeBuffer(delim - start + 1);

        start = buffer_.GetContentStart();
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

        buffer_.ConsumeBuffer(delim - start + 1);
        start = buffer_.GetContentStart();

        const char* ctrl= HTTP_CTRL;
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

        buffer_.ConsumeBuffer(sizeof(http_version) - 1 + ctrl_len);

        FinishParsingRequestLine();
        return true;
    }

    return false;
}

bool HttpReader::ParseHeader()
{
    const char* start = buffer_.GetContentStart();
    const char* end   = buffer_.GetContentEnd();

    static const char* ctrl= HTTP_CTRL;
    static const size_t ctrl_len = strlen(ctrl);
    static const char* end_of_ctrl = ctrl + ctrl_len;

    static const char* header_delim = HTTP_HEADER_DELIM;
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
            buffer_.ConsumeBuffer(line_end - start + ctrl_len);
            break;
        }

        std::string key(start, colon);
        std::string value(colon + 2, line_end);

        request_.AddHeader(key.c_str(), value.c_str());

        buffer_.ConsumeBuffer(line_end - start + ctrl_len);
        start = buffer_.GetContentStart();
    }

    std::string connection = request_.GetHeaderValue("Connection");

    keepalive_ = (connection == "Keep-Alive");
    FinishParsingHeader();
    return true;
}


bool HttpReader::ParseBody()
{
   const char* start = buffer_.GetContentStart();
   const char* end   = buffer_.GetContentEnd();

   if (request_.GetHttpMethod() != HttpRequest::HM_POST)
   {
       buffer_.ConsumeBuffer(end - start);
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

   buffer_.ConsumeBuffer(end - start);

   return true;
}

////////////////HTTP WRITER/////////////////////////////

HttpWriter::HttpWriter(HttpConnection& conn)
    :conn_(conn)
{
}

HttpWriter::~HttpWriter()
{
    ResetWriter();
}

void HttpWriter::ResetWriter()
{
    SocketBufferNode* tmp = pendingWrite_.PopFrontNode();
    while (tmp)
    {
        buffer_.ReleaseWriteBuffer(tmp);
        tmp = pendingWrite_.PopFrontNode();
    }
}

SocketBufferNode* HttpWriter::AllocWriteBuffer(int sz)
{
    return buffer_.AllocWriteBuffer(sz);
}

void HttpWriter::AddToWriteList(SocketBufferNode* node)
{
    pendingWrite_.AppendBufferNode(node);
}

int HttpWriter::SendPendingBuffer()
{
    int co = 0;
    SocketBufferNode* tmp = pendingWrite_.GetFrontNode();
    while (tmp)
    {
        int sz = conn_.SendBuffer(tmp->curPtr_, tmp->curSize_);

        if (sz == tmp->curSize_)
        {
            co += sz;
            pendingWrite_.PopFrontNode();
            buffer_.ReleaseWriteBuffer(tmp);
            tmp = pendingWrite_.GetFrontNode();
        }
        else if (sz >= 0)
        {
            co += sz;
            tmp->curPtr_ += sz;
            tmp->curSize_ -= sz;
            break;
        }
        else
        {
            // error
            slog(LOG_ERROR, "send buffer error, sock(%d)", conn_.GetConnectionId());
            break;
        }
    }

    return co;
}

