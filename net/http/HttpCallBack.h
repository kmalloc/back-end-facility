#ifndef __HTTP_CALL_BACK_H__
#define __HTTP_CALL_BACK_H__

#include <map>
#include <string>

#include "net/http/HttpRequest.h"
#include "net/http/HttpResponse.h"

typedef void (* HttpCallBack)(const HttpRequest& req, HttpResponse& reponse);

#endif

