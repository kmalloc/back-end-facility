#include "gtest.h"

#include "net/http/HttpContext.h"

// only request line.
static const char* fake_http_req_get = "GET http://my.localhost.hl?test=v&name=miliao HTTP/1.1\r\n\r\n";


static const char* fake_http_req_post_req = "POST http://my.localhost.cc?test=p&name=miliao2 HTTP/1.1\r\n";
static const char* fake_http_req_post_header =
"Accept: image/git, image/x-xbitmap, image/jpeg\r\n\
Referer: xxxx.xxx\r\n\
Accept-Language: en-us\r\n\
UA-CPU: x86\r\n\
Accept-Encoding: gzip, deflate\r\n\
Connection: Keep-Alive\r\n\
Content-Length: 32\r\n\
Host: www.xmalloc.com\r\n\
Cookie: utmc=226521935; utma=226521935.31911776\r\n\
\r\n";

static const char* fake_http_req_post_body = "abcdabcdabcdabcdabcdabcdabcdabcd";

static int g_curr_req = 0;

static void requestHandler(const HttpRequest& req, HttpResponse& response)
{
    response.SetCloseConn(false);

    if (g_curr_req == 0)
    {
        EXPECT_EQ(HttpRequest::HM_GET, req.GetHttpMethod());

        ASSERT_STREQ("GET", req.GetHttpMethodName().c_str());

        EXPECT_EQ(HttpRequest::HV_11, req.GetVersion());
        ASSERT_STREQ("HTTP/1.1", req.GetVersionName());

        ASSERT_STREQ("http://my.localhost.hl", req.GetUrl().c_str());
        ASSERT_STREQ("test=v&name=miliao", req.GetUrlData().c_str());

        EXPECT_EQ(0, req.GetHeader().size());
        EXPECT_EQ(0, req.GetBodyLength());
    }
    else
    {
        ASSERT_STREQ("POST", req.GetHttpMethodName().c_str());
        EXPECT_EQ(HttpRequest::HM_POST, req.GetHttpMethod());

        ASSERT_STREQ("HTTP/1.1", req.GetVersionName());

        ASSERT_STREQ("http://my.localhost.cc", req.GetUrl().c_str());
        ASSERT_STREQ("test=p&name=miliao2", req.GetUrlData().c_str());

        std::map<std::string, std::string> headers = req.GetHeader();
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

        EXPECT_EQ(32, req.GetBodyLength());
        ASSERT_STREQ("abcdabcdabcdabcdabcdabcdabcdabcd", req.GetHttpBody().c_str());
    }
}

TEST(testParsingRequest, HttpContextTest)
{
    HttpServer server("127.0.0.1");
    LockFreeBuffer buff_alloc(32, 512);

    int fake_conn_id = 12;

    g_curr_req = 0;
    HttpContext context(server, buff_alloc, &requestHandler);

    // test GET
    context.ResetContext(fake_conn_id);
    context.AppendData(fake_http_req_get, strlen(fake_http_req_get));
    context.ProcessHttpRequest();

    EXPECT_EQ(HttpContext::HS_INVALID, context.ParsingStage());
    EXPECT_FALSE(context.IsKeepAlive());

    context.ReleaseContext();


    // test POST

    context.ResetContext(fake_conn_id + 1);
    g_curr_req = 1;

    int req_len = strlen(fake_http_req_post_req);
    int header_len = strlen(fake_http_req_post_header);
    int body_len = strlen(fake_http_req_post_body);

    int half_header = header_len/2;
    int half_body = body_len/2;

    context.AppendData(fake_http_req_post_req, req_len);
    context.ProcessHttpRequest();

    context.AppendData(fake_http_req_post_header, half_header);
    context.ProcessHttpRequest();
    context.AppendData(fake_http_req_post_header + half_header, header_len - half_header);
    context.ProcessHttpRequest();

    context.AppendData(fake_http_req_post_body, half_body);
    context.ProcessHttpRequest();
    context.AppendData(fake_http_req_post_body + half_body, body_len - half_body);
    context.ProcessHttpRequest();

    EXPECT_EQ(HttpContext::HS_INVALID, context.ParsingStage());
    EXPECT_TRUE(context.IsKeepAlive());

    context.ReleaseContext();
}


