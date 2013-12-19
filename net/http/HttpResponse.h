#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__

#include <map>
#include <stdio.h>

class HttpResponse
{
    public:

        enum HttpStatusCode
        {
            HSC_UNKNOWN,
            HSC_200 = 200, // sucess
            HSC_400 = 400, // bad request
            HSC_401 = 401, // unauthorized
            HSC_403 = 403, // forbidden
            HSC_404 = 404, // not found
            HSC_500 = 500, // internal error
            HSC_503 = 503, // unavailable
        };

    public:

        HttpResponse() {}

        void SetCloseConn(bool close) { closeConn_ = close; }
        void SetStatusCode(HttpStatusCode code) { statusCode_ = code; }

        void AddHeader(const char* key, const char* value) { httpHeader_[key] = value; }

        void SetStatusMessage(const char* msg) { statusMsg_ = msg; }
        void SetBody(const char* body) { httpBody_ = body; }

        bool GenerateResponse(char* buffer, size_t size) const
        {
            size_t sz = snprintf(buffer, 32, "HTTP/1.1 %d", statusCode_);

            size_t tmp = statusMsg_.size();
            if (tmp + sz + 32> size) return false;

            memcpy(buffer + sz, statusMsg_.c_str(), tmp + 1);

            strcat(buffer, "\r\n");
            if (closeConn_)
            {
                const char* close_info = "Connection close\r\n";

                tmp = strlen(close_info);
                memcpy(buffer + sz, close_info, tmp + 1);

                sz += tmp;
            }
            else
            {
                for (std::map<std::string, std::string>::const_iterator it = httpHeader_.begin();
                        it != httpHeader_.end(); ++it)
                {
                    tmp = strlen(it->first);
                    if (sz + tmp + 1 >= size) return;

                    memcpy(buffer + sz, it->first, tmp + 1);
                    sz += tmp;

                    if (sz + 3 > size) return;

                    memcpy(buffer + sz, ": ", 3);

                    tmp = strlen(it->second);
                    if (sz + tmp + 1 >= size) return;

                    memcpy(buffer + sz, it->second, tmp + 1);
                }
            }

            memcpy(buffer + sz, "\r\n", 3);

            tmp = httpBody_.size();

            if (sz + tmp + 1 >= size) return false;

            memcpy(buffer + sz, httpBody_.c_str(), tmp + 1);

            return true;
        }

    private:

        bool closeConn_;
        HttpStatusCode statusCode_;
        std::string statusMsg_;
        std::string httpBody_;
        std::map<std::string, std::string> httpHeader_;
}

#endif

