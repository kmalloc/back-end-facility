#include "LogStream.h"

#include <algorithm>

namespace slog {

static const char g_digits[] = "9876543210123456789";
static const char* g_zero = g_digits + 9;
BOOST_STATIC_ASSERT(sizeof(g_digits) == 20);

const char g_digits_hex[] = "0123456789ABCDEF";
BOOST_STATIC_ASSERT(sizeof g_digits_hex == 17);

// Efficient Integer to String Conversions, by Matthew Wilson.
template<typename T>
inline size_t convert(char buf[], T value)
{
    T i = value;
    char* p = buf;

    do
    {
        int lsd = static_cast<int>(i % 10);
        i /= 10;
        *p++ = g_zero[lsd];
    } while (i != 0);

    *p = '\0';
    return p - buf;
}

size_t LogStream::Convert64(int64_t value)
{
    if (m_size - m_cur < 32) return 0;

    char* ps = m_buff + m_cur;
    size_t len = convert(ps, value);

    m_cur += len;
    if (value < 0)
    {
        m_buff[m_cur++] = '-';
        m_buff[m_cur] = '\0';
    }

    std::reverse(ps, m_buff + m_cur);
    return m_buff + m_cur - ps;
}

size_t LogStream::Convert64U(uint64_t value)
{
    if (m_size - m_cur < 32) return 0;

    char* ps = m_buff + m_cur;
    size_t len = convert(ps, value);

    m_cur += len;
    std::reverse(ps, m_buff + m_cur);

    return len;
}

size_t LogStream::ConvertHex(uintptr_t value)
{
    uintptr_t i = value;
    char* p = m_buff + m_cur;
    char* buff = p;

    do
    {
        int lsd = static_cast<int>(i % 16);
        i /= 16;
        *p++ = g_digits_hex[lsd];
    } while (i != 0);

    memcpy(p, "x0", 2);
    p += 2;

    *p = '\0';
    std::reverse(buff, p);

    size_t len = p - buff;
    m_cur += len;

    return len;
}

}  // namespace slog
