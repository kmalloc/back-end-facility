#ifndef __HTTP_RESPONSE_H__
#define __HTTP_RESPONSE_H__

#include <map>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

        HttpResponse(): response_(false)
        {
            msgLen_ = 13 + 2; //HTTP/1.1 404\r\n
        }

        void SetShouldResponse(bool respone) { response_ = respone; }
        bool GetShouldResponse() const { return response_; }

        void SetCloseConn(bool close) { closeConn_ = close; }
        void SetStatusCode(HttpStatusCode code) { statusCode_ = code; }

        bool ShouldCloseConnection() const { return closeConn_; }

        void AddHeader(const char* key, const char* value)
        {
            msgLen_ += strlen(key);
            msgLen_ += 2; //": "
            msgLen_ += strlen(value);
            msgLen_ += 2; //"\r\n"

            httpHeader_[key] = value;
        }

        void SetStatusMessage(const char* msg)
        {
            statusMsg_ = msg;
        }

        void SetBody(const char* body) { httpBody_ = body; }

        size_t GenerateResponse(char* buffer, size_t size) const
        {
            size_t sz = snprintf(buffer, 32, "HTTP/1.1 %d ", statusCode_);
            size_t tmp = statusMsg_.size();

            if (tmp + sz + 2 >= size) return false;

            memcpy(buffer + sz, statusMsg_.c_str(), tmp + 1);

            strcat(buffer, "\r\n");

            sz += tmp + 2;

            for (std::map<std::string, std::string>::const_iterator it = httpHeader_.begin();
                    it != httpHeader_.end(); ++it)
            {
                tmp = it->first.size();
                if (sz + tmp + 1 >= size) return false;

                memcpy(buffer + sz, it->first.c_str(), tmp + 1);
                sz += tmp;

                if (sz + 3 > size) return false;

                memcpy(buffer + sz, ": ", 3);
                sz += 2;

                tmp = it->second.size();
                if (sz + tmp + 3 >= size) return false;

                memcpy(buffer + sz, it->second.c_str(), tmp + 1);
                memcpy(buffer + sz + tmp, "\r\n", 3);

                sz += tmp + 2;
            }

            memcpy(buffer + sz, "\r\n", 3);

            sz += 2;

            tmp = httpBody_.size();

            if (sz + tmp + 1 >= size) return false;

            memcpy(buffer + sz, httpBody_.c_str(), tmp + 1);

            sz += tmp;

            return sz;
        }

        void CleanUp()
        {
            statusMsg_ = "";
            httpBody_  = "";
            httpHeader_.clear();
        }

        size_t GetResponseSize() const { return msgLen_ + statusMsg_.size() + 2 + httpBody_.size() + 2; }

    private:

        bool response_;
        bool closeConn_;

        size_t msgLen_;

        HttpStatusCode statusCode_;
        std::string statusMsg_;
        std::string httpBody_;
        std::map<std::string, std::string> httpHeader_;
};

#endif

