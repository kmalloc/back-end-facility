#ifndef SLOG_LOGSTREAM_H_
#define SLOG_LOGSTREAM_H_

#include <string>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <boost/noncopyable.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_signed.hpp>
#include <boost/type_traits/is_arithmetic.hpp>

namespace slog {

class LogStream
{
    public:
        LogStream(char* buff, size_t size): m_buff(buff), m_cur(0), m_size(size) {}

        void Reset() { m_cur = 0; m_buff[0] = 0; }
        size_t Length() const { return m_cur; }
        const char* Buffer() const { return m_buff; }
        void EmptyBuff() { memset(m_buff, 0, m_size); }
        void SetTrailingNull() { m_buff[m_cur] = 0; }

        size_t Append(const char* data, size_t len)
        {
            assert(m_size - m_cur >= len);

            memcpy(m_buff + m_cur, data, len);
            m_cur += len;

            return len;
        }

        // note that, this is function much slower than Append
        template<typename T>
        int FmtAppend(const char* fmt, T v)
        {
            int left = m_size - m_cur;
            int n = snprintf(m_buff + m_cur, left, fmt, v);
            if (n <= 0) return 0;

            m_cur += n;
            return n;
        }

        // for formatting arithmetic value
        // eg, unsigned char, int, unsigned int, short, etc.
        template<typename T>
        LogStream& operator<<(T v)
        {
            BOOST_STATIC_ASSERT(boost::is_arithmetic<T>::value);
            AppendArithm(v, select_flag<boost::is_signed<T>::value>());
            return *this;
        }

        LogStream& operator<<(const std::string& v)
        {
            Append(v.data(), v.size());
            return *this;
        }

        template <size_t N>
        LogStream& operator<<(char(&v)[N])
        {
            BOOST_STATIC_ASSERT(N);
            Append(v, N-1);
            return *this;
        }

    private:
        size_t Convert64(int64_t value);
        size_t Convert64U(uint64_t value);
        size_t ConvertHex(uintptr_t value);

        template<bool> struct select_flag {};

        // format unsigned signed value
        template<typename T>
        size_t AppendArithm(T n, select_flag<false>)
        {
            return Convert64U(static_cast<uint64_t>(n));
        }

        // format signed value
        template<typename T>
        size_t AppendArithm(T n, select_flag<true>)
        {
            return Convert64(static_cast<int64_t>(n));
        }

    private:
        char* m_buff;
        size_t m_cur;
        const size_t m_size;
};

class LogStreamFixed: public LogStream, private boost::noncopyable
{
    public:
        static const int S_MAX_BUFF_SZ = 4 * 1024;
        LogStreamFixed(): LogStream(m_buff, S_MAX_BUFF_SZ) { m_buff[0] = 0; }

    private:
        char m_buff[S_MAX_BUFF_SZ];
};

template<>
inline LogStream& LogStream::operator<<(double v)
{
    FmtAppend("%.12g", v);
    return *this;
}

template<>
inline LogStream& LogStream::operator<<(const void* v)
{
    ConvertHex(reinterpret_cast<uintptr_t>(v));
    return *this;
}

template<>
inline LogStream& LogStream::operator<<(void* v)
{
    ConvertHex(reinterpret_cast<uintptr_t>(v));
    return *this;
}

template<>
inline LogStream& LogStream::operator<<(const char* v)
{
    Append(v, strlen(v));
    return *this;
}

template<>
inline LogStream& LogStream::operator<<(char* v)
{
    Append(v, strlen(v));
    return *this;
}

template<>
inline LogStream& LogStream::operator<<(char v)
{
    Append(&v, 1);
    return *this;
}

template<>
inline LogStream& LogStream::operator<<(bool v)
{
    Append(v?"1":"0", 1);
    return *this;
}

inline LogStream& operator<<(LogStream& ls, LogStream&(*f)(LogStream&))
{
    return f(ls);
}

}  // namespace slog

namespace std {
    inline slog::LogStream& endl(slog::LogStream& ls)
    {
        ls.Append("\n", 1);
        return ls;
    }
}

#endif  // SLOG_LOGSTREAM_H_
