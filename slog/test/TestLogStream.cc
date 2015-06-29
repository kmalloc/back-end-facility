#include <gtest/gtest.h>

#include <string>
#include <iostream>
#include "LogStream.h"

using namespace std;
using namespace slog;

TEST(test_logstream, test_append)
{
    LogStreamFixed ls;
    string d1("abcd1234");
    ls.Append(d1.c_str(), d1.size() + 1);

    ASSERT_STREQ(ls.Buffer(), d1.c_str());
    ASSERT_EQ(ls.Length(), d1.size() + 1);

    string d2("vvwww", 10);
    ls.Append(d2.c_str(), d2.size());
    ASSERT_EQ(ls.Length(), d1.size() + 1 + d2.size());

    LogStreamFixed fmt;

    fmt.FmtAppend("%s", "1234abcd");
    ASSERT_EQ(fmt.Length(), 8u);
    ASSERT_STREQ(fmt.Buffer(), "1234abcd");
    fmt.FmtAppend("%03d", 233);
    ASSERT_STREQ(fmt.Buffer(), "1234abcd233");

    void* p = reinterpret_cast<void*>(0x12345);
    fmt.FmtAppend("%p", p);
    ASSERT_STREQ(fmt.Buffer(), "1234abcd2330x12345");

    fmt.FmtAppend("int:%d", 233);
    ASSERT_STREQ(fmt.Buffer(), "1234abcd2330x12345int:233");
    fmt.FmtAppend("unsigned int:%d", 233);
    ASSERT_STREQ(fmt.Buffer(), "1234abcd2330x12345int:233unsigned int:233");

    fmt.FmtAppend("float:%5.5f", 233.222222);
    ASSERT_STREQ(fmt.Buffer(), "1234abcd2330x12345int:233unsigned int:233float:233.22222");

    fmt.Append("gggg", 5);
    ASSERT_STREQ(fmt.Buffer(), "1234abcd2330x12345int:233unsigned int:233float:233.22222gggg");
}

TEST(test_logstream, test_operator)
{
    LogStreamFixed ls;

    ls.EmptyBuff();

    ls << 'a' << 'b' << 'c';
    ASSERT_STREQ(ls.Buffer(), "abc");

    ls << true;
    ASSERT_STREQ(ls.Buffer(), "abc1");

    ls << 23;
    ASSERT_STREQ(ls.Buffer(), "abc123") << "sz:" << ls.Length();

    ls << (short)45;
    ASSERT_STREQ(ls.Buffer(), "abc12345") << "sz:" << ls.Length();

    ls << (unsigned short)67;
    ASSERT_STREQ(ls.Buffer(), "abc1234567") << "sz:" << ls.Length();

    ls << (int64_t)89;
    ASSERT_STREQ(ls.Buffer(), "abc123456789") << "sz:" << ls.Length();

    ls << (unsigned int64_t)10;
    ASSERT_STREQ(ls.Buffer(), "abc12345678910") << "sz:" << ls.Length();

    ls << (unsigned int)-1;
    ASSERT_STREQ(ls.Buffer(), "abc123456789104294967295") << "sz:" << ls.Length();

    ls << -1;
    ASSERT_STREQ(ls.Buffer(), "abc123456789104294967295-1") << "sz:" << ls.Length();

    ls << "oooppp";
    ASSERT_STREQ(ls.Buffer(), "abc123456789104294967295-1oooppp") << "sz:" << ls.Length();

    const void* p = reinterpret_cast<const void*>(0x12345);
    ls << p;
    ASSERT_STREQ(ls.Buffer(), "abc123456789104294967295-1oooppp0x12345")
        << "sz:" << ls.Length();

    ls << string("xxxx");
    ASSERT_STREQ(ls.Buffer(), "abc123456789104294967295-1oooppp0x12345xxxx")
        << "sz:" << ls.Length();

    ls << 12.2222;
    ASSERT_STREQ(ls.Buffer(), "abc123456789104294967295-1oooppp0x12345xxxx12.2222")
        << "sz:" << ls.Length();

    ls << -2 << 's' << 233;
    ASSERT_STREQ(ls.Buffer(), "abc123456789104294967295-1oooppp0x12345xxxx12.2222-2s233")
        << "sz:" << ls.Length();

    ls.Reset();
    ls.EmptyBuff();

    char arr[] = "wwww";
    ls << 233 << ',' << -233 << 'A' << " empty " <<
        (reinterpret_cast<void*>(7777)) << string(" string ") << arr;
    ASSERT_STREQ(ls.Buffer(), "233,-233A empty 0x1E61 string wwww");
}

