#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include <map>
#include <string>
#include <stdlib.h>

class HttpRequest
{
    public:

        enum HttpMethod
        {
            HM_GET,
            HM_POST,
            HM_HEAD,
            HM_PUT,
            HM_DELETE,
            HM_INVALID
        };

        enum HttpVersion
        {
            HV_10,
            HV_11,
            HV_INVALID
        };

        static const size_t MaxBodyLength = 4096; // 4mb

    public:

        HttpRequest()
            : bodyLen_(0)
            , method_(HM_INVALID)
            , version_(HV_INVALID)
        {
        }

        bool SetHttpMethod(const char* start, const char* end)
        {
            std::string str(start, end);
            return SetHttpMethod(str);
        }

        bool SetHttpMethod(const std::string& method)
        {
            method_ = HM_INVALID;
            if (method == "GET")
                method_ = HM_GET;
            else if (method == "POST")
                method_ = HM_POST;
            else if (method ==  "HEAD")
                method_ = HM_HEAD;
            else if (method == "PUT")
                method_ = HM_PUT;
            else if (method == "DELETE")
                method_ = HM_DELETE;

            return method_ != HM_INVALID;
        }

        std::string GetHttpMethodName() const
        {
            std::string result = "UNKNOWN";
            switch(method_)
            {
                case HM_GET:
                    result = "GET";
                    break;
                case HM_POST:
                    result = "POST";
                    break;
                case HM_HEAD:
                    result = "HEAD";
                    break;
                case HM_PUT:
                    result = "PUT";
                    break;
                case HM_DELETE:
                    result = "DELETE";
                    break;
                default:
                    break;
            }

            return result;
        }

        HttpMethod GetHttpMethod() const { return method_; }

        bool SetVersion(const std::string& version)
        {
            version_ = HV_INVALID;

            if (version == "HTTP/1.1") version_ = HV_11;
            else if (version == "HTTP/1.0") version_ = HV_10;

            return version_ != HV_INVALID;
        }

        const char* GetVersionName() const
        {
            if (version_ == HV_10) return "HTTP/1.0";
            else if (version_ == HV_11) return "HTTP/1.1";

            return "UNKNOWN";
        }

        HttpVersion GetVersion() const
        {
            return version_;
        }

        void SetUrl(const std::string& url)
        {
            reqUrl_ = url;
        }

        const std::string& GetUrl() const
        {
            return reqUrl_;
        }

        void SetUrlData(const std::string& data)
        {
            urlData_ = data;
        }

        const std::string& GetUrlData() const
        {
            return urlData_;
        }

        void SetBodySize(size_t sz)
        {
            bodyLen_ = sz;
            httpBody_.reserve(sz);
        }

        // length of data received.
        size_t GetCurBodyLength() const
        {
            return httpBody_.size();
        }

        size_t GetBodyLength() const
        {
            return bodyLen_;
        }

        const std::string& GetHttpBody() const
        {
            return httpBody_;
        }

        void AppendBody(const char* data)
        {
            httpBody_.append(data);
        }

        void AppendBody(const char* start, const char* end)
        {
            if (end - start >= httpBody_.capacity()) return;

            httpBody_.append(start, end);
        }

        void AddHeader(const char* key, const char* value) { httpHeader_[key] = value; }
        const std::map<std::string, std::string>& GetHeader() const { return httpHeader_; }

        std::string GetHeaderValue(const std::string& key) const
        {
            std::map<std::string, std::string>::const_iterator it = httpHeader_.find(key);
            if (it == httpHeader_.end()) return "";

            return it->second;
        }

        void CleanUp()
        {
            reqUrl_ = "";
            urlData_ = "";
            httpBody_ = "";
            httpHeader_.clear();
            bodyLen_ = 0;
        }

    private:

        size_t bodyLen_;
        HttpMethod method_;
        HttpVersion version_;

        std::string reqUrl_;
        std::string urlData_; // data after url in POST request
        std::string httpBody_;
        std::map<std::string, std::string> httpHeader_;
};

#endif

