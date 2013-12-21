#include "gtest.h"

#include "net/http/HttpContext.h"

static const char* fake_http_req_get = "GET http://my.localhost.hl HTTP/1.1\r\n";
static const char* fake_http_req_post = "POST http://my.loclahost.cc HTTP/1.1\r\n\
                                   Accept: image/git, image/x-xbitmap, image/jpeg\r\n\
                                   Referer: xxxx.xxx\r\n\
                                   Accept-Language: en-us\r\n\
                                   UA-CPU: x86\r\n\
                                   Accept-Encoding: gzip, deflate\r\n\
                                   Connection: Keep-Alive\r\n\
                                   Content-Length: 32\r\n\
                                   Host: www.xmalloc.com\r\n\
                                   Cookie: utmc=226521935; utma=226521935.31911776\r\n\
                                   \r\n\
                                   abcdabcdabcdabcdabcdabcdabcdabcd";



static void requestHandler(const HttpRequest& req, HttpResponse& response)
{
}

TEST(testParsingRequest, HttpContextTest)
{
   SocketServer server;
   LockFreeBuffer buff_alloc(32, 512);

   int fake_conn_id = 12;

   HttpContext context(server, buff_alloc, &requestHandler);
   context.ResetContext(fake_conn_id);

   context.AppendData(fake_http_req_get, strlen(fake_http_req_get));
   context.RunParser();
}


