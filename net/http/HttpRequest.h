#ifndef __HTTP_REQUEST_H__
#define __HTTP_REQUEST_H__

#include <map>
#include <string>

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

    public:

        HttpRequest();

        bool SetHttpMethod(const std::string& method) const
        {
            method_ = HM_INVALID;
            switch(method)
            {
                case "GET":
                    method_ = HM_GET;
                    break;
                case "POST":
                    method_ = HM_POST;
                    break;
                case "HEAD":
                    method_ = HM_HEAD;
                    break;
                case "PUT":
                    method_ = HM_PUT;
                    break;
                case "DELETE":
                    method_ = HM_DELETE;
                    break;
                default:
                    break;
            }

            return method_ != HM_INVALID;
        }

        const char* GetHttpMethod() const
        {
            const char* result = "UNKNOWN";
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

        bool SetVersion(const std::string& version)
        {
            version_ = HV_INVALID;

            if (version == "HTTP/1.1") version_ = HV_11;
            else if (version == "HTTP/1.0") version_ = HV_10;

            return version_ != HV_INVALID;
        }

        const char* GetVersion()
        {
            if (version_ == HV_10) return "HTTP/1.0";
            else if (version_ == HV_11) return "HTTP/1.1";

            return "UNKNOWN";
        }

        void SetUrl(const std::string& url)
        {
            reqUrl_ = url;
        }

        void SetPostData(const std::string& data)
        {
            postData_ = data;
        }

        void AddHeader(const char* key, const char* value) { httpHeader_[key] = value; }
        const std::map<std::string, std::string>& GetHeader() const { return httpHeader_; }

        void CleanUp()
        {
            httpBody_ = "";
            httpHeader_.clear();
        }

    private:

        HttpMethod method_;
        HttpVersion version_;

        std::string reqUrl_;
        std::string postData_; // data after url in POST request
        std::string httpBody_;
        std::map<std::string, std::string> httpHeader_;
}

#endif

