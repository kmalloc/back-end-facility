#include "CacheTimeStamp.h"

#include <stdio.h>
#include <assert.h>
#include <sys/time.h>

namespace slog {

static __thread time_t tl_last_sec = 0;
static __thread char tl_time_str[32] = {0};

CacheTimeStamp::CacheTimeStamp(int64_t micro_sec)
    :m_micro_sec(micro_sec), m_trailing_ms(micro_sec%S_MS_PER_SEC)
{
    time_t sec = static_cast<time_t>(micro_sec/S_MS_PER_SEC);
    if (tl_last_sec != sec)
    {
        struct tm t_tm;
        tl_last_sec = sec;

        localtime_r(&sec, &t_tm);
        int len = snprintf(tl_time_str, sizeof tl_time_str, "%4d_%02d_%02d-%02d_%02d_%02d",
                t_tm.tm_year + 1900, t_tm.tm_mon + 1, t_tm.tm_mday,
                t_tm.tm_hour, t_tm.tm_min, t_tm.tm_sec);

        assert(len == FmtSecLength());
    }
}

std::string CacheTimeStamp::ToString() const
{
    char buf[16] = {0};
    std::string s(tl_time_str);
    snprintf(buf, 16, ":%06d", static_cast<int>(TrailingMicroSec()));
    return s + buf;
}

const char* CacheTimeStamp::GetFmtSec() const
{
    return tl_time_str;
}

CacheTimeStamp CacheTimeStamp::Now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    int64_t sec = tv.tv_sec;
    return CacheTimeStamp(sec * S_MS_PER_SEC + tv.tv_usec);
}

}  // namespace slog

