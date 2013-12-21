#include "gtest.h"

#include "net/http/HttpContext.h"

static const char* fake_http_req_get = "GET http://my.localhost.hl?test=v&name=miliao HTTP/1.1\r\n";
static const char* fake_http_req_post = "POST http://my.localhost.cc?test=p&name=miliao2 HTTP/1.1\r\n\
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
    response.SetCloseConn(false);
}

TEST(testParsingRequest, HttpContextTest)
{
   SocketServer server;
   LockFreeBuffer buff_alloc(32, 512);

   int fake_conn_id = 12;

   HttpContext context(server, buff_alloc, &requestHandler);

   // test GET
   context.ResetContext(fake_conn_id);

   context.AppendData(fake_http_req_get, strlen(fake_http_req_get));
   context.RunParser();

   HttpRequest req_get = context.GetRequest();
   EXPECT_EQ(HttpRequest::HM_GET, req_get.GetHttpMethod());

   ASSERT_STREQ("GET", req_get.GetHttpMethodName().c_str());


   EXPECT_EQ(HttpRequest::HV_11, req_get.GetVersion());
   ASSERT_STREQ("HTTP/1.1", req_get.GetVersionName());

   ASSERT_STREQ("http://my.localhost.hl", req_get.GetUrl().c_str());
   ASSERT_STREQ("test=v&name=miliao", req_get.GetUrlData().c_str());

   EXPECT_EQ(0, req_get.GetHeader().size());
   EXPECT_EQ(0, req_get.GetBodyLength());

   EXPECT_EQ(HttpContext::HS_INVALID, context.ParsingStage());
   EXPECT_FALSE(context.IsKeepAlive());

   context.ReleaseContext();


   // test POST

   context.ResetContext(fake_conn_id + 1);
   context.AppendData(fake_http_req_post, strlen(fake_http_req_post));
   context.RunParser();

   HttpRequest req_post = context.GetRequest();
   ASSERT_STREQ("POST", req_post.GetHttpMethodName().c_str());
   EXPECT_EQ(HttpRequest::HM_POST, req_post.GetHttpMethod());

   ASSERT_STREQ("HTTP/1.1", req_post.GetVersionName());

   ASSERT_STREQ("http://my.localhost.cc", req_post.GetUrl().c_str());
   ASSERT_STREQ("test=p&name=miliao2", req_post.GetUrlData().c_str());

   std::map<std::string, std::string> headers = req_post.GetHeader();
   EXPECT_EQ(9, headers.size());


   /*
    * Accept: image/git, image/x-xbitmap, image/jpeg\r\n\
    * Referer: xxxx.xxx\r\n\
    * Accept-Language: en-us\r\n\
    * UA-CPU: x86\r\n\
    * Accept-Encoding: gzip, deflate\r\n\
    * Connection: Keep-Alive\r\n\
    * Content-Length: 32\r\n\
    * Host: www.xmalloc.com\r\n\
    * Cookie: utmc=226521935; utma=226521935.31911776\r\n\
    */

   ASSERT_STREQ(headers["Accept"].c_str(), "image/git, image/x-xbitmap, image/jpeg");
   ASSERT_STREQ(headers["Accept-Language"].c_str(), "en-us");
   ASSERT_STREQ(headers["UA-CPU"].c_str(),"x86");
   ASSERT_STREQ(headers["Accept-Encoding"].c_str(), "gzip, deflate");
   ASSERT_STREQ(headers["Connection"].c_str(), "Keep-Alive");
   ASSERT_STREQ(headers["Content-Length"].c_str(), "32");
   ASSERT_STREQ(headers["Host"].c_str(), "www.xmalloc.com");
   ASSERT_STREQ(headers["Cookie"].c_str(),"utmc=226521935; utma=226521935.31911776");

   EXPECT_EQ(32, req_post.GetBodyLength());
   ASSERT_STREQ("abcdabcdabcdabcdabcdabcdabcdabcd", req_post.GetHttpBody().c_str());

   EXPECT_EQ(HttpContext::HS_INVALID, context.ParsingStage());
   EXPECT_TRUE(context.IsKeepAlive());

   context.ReleaseContext();
}


