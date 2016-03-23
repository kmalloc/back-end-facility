#include "Logger.h"

#include <string>
#include <vector>

using namespace std;
using namespace slog;

static int dummy_func()
{
    static int s_i = 42;
    if (s_i % 2) SLOG_INFO << "logging from initializing global value:" << s_i;

    SLOG_ERROR << "logging from initializing global value:" << s_i;

    return s_i++;
}

static int g_int1 = dummy_func();
static int g_int2 = dummy_func();
static int g_int3 = dummy_func();
static int g_int4 = dummy_func();

int main()
{
    Logger::InitLogging("./dummy.log", Logger::LL_DEBUG);

    void* p = &g_int1;
    const void* p2 = p;

    SLOG_INFO << "info-level log:" << 42 << 'a' << 23.33;
    SLOG_WARN << "warn-level log:" << "vvv, p:" << p << endl;
    SLOG_TRACE << "trace-level log:" << "will not print, p:" << p;

    Logger::SetLogLevel(Logger::LL_TRACE);

    SLOG_WARN << "warn-level log22:" << "vvv, pointer:" << p2 << endl;
    SLOG_TRACE << "current log level is changed to trace, so trace-level log:" << "will print:" << p;

    return 0;
}

