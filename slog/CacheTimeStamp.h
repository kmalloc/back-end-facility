#ifndef SLOG_CACHETIMESTAMP_H_
#define SLOG_CACHETIMESTAMP_H_

#include <string>
#include <stdint.h>

namespace slog {

class CacheTimeStamp
{
    public:
        explicit CacheTimeStamp(int64_t micro_sec);

        std::string ToString() const;
        static CacheTimeStamp Now();
        const char* GetFmtSec() const;

        static int FmtSecLength() { return 19; }
        int64_t GetMicroSecSinceEpoch() const { return m_micro_sec; }
        int64_t TrailingMicroSec() const { return m_trailing_ms; }

    public:
        static const int S_MS_PER_SEC = 1000 * 1000;

    private:
        const int64_t m_micro_sec;
        const int64_t m_trailing_ms;
};

inline double operator-(const CacheTimeStamp& t1, const CacheTimeStamp& t2)
{
    int64_t diff = t1.GetMicroSecSinceEpoch() - t2.GetMicroSecSinceEpoch();
    return (static_cast<double>(diff))/CacheTimeStamp::S_MS_PER_SEC;
}

}  // namespace slog

#endif  //

