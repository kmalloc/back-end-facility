#include <gtest/gtest.h>

#include <string>
#include <iostream>
#include "CacheTimeStamp.h"

using namespace std;
using namespace slog;

TEST(test_cachetimestamp, test_sec_mircosec)
{
    CacheTimeStamp t0 = CacheTimeStamp::Now();
    const char* fmt_sec0 = t0.GetFmtSec();
    string fmt_str0(fmt_sec0);
    cout << "current timestamp:" << fmt_str0 << endl;
    string fmt_full_0 = t0.ToString();
    cout << "current timestamp full format:" << fmt_full_0 << endl;

    int64_t ms = 1682079033562*1000 + 233;
    CacheTimeStamp t1(ms);
    ASSERT_EQ(t1.TrailingMicroSec(), ms % t1.S_MS_PER_SEC);

    const char* fmt_sec1 = t1.GetFmtSec();
    string fmt_str1(fmt_sec1);
    string fmt_full_1 = t1.ToString();
    ASSERT_STREQ(fmt_full_1.c_str(), "2023_04_21-20_10_33:562233") << "full format string:" << fmt_full_1 << endl;
    ASSERT_STREQ(fmt_str1.c_str(), "2023_04_21-20_10_33") << "fmt str1:" << fmt_str1 << endl;

    CacheTimeStamp t2(ms + 233);
    const char* fmt_sec2 = t2.GetFmtSec();
    string fmt_str2(fmt_sec2);
    string fmt_full_2 = t2.ToString();

    ASSERT_EQ(fmt_str1, fmt_str2);
    ASSERT_EQ(t2.TrailingMicroSec() - t1.TrailingMicroSec(), 233);
    ASSERT_STREQ(fmt_str2.c_str(), "2023_04_21-20_10_33");
    ASSERT_STREQ(fmt_full_2.c_str(), "2023_04_21-20_10_33:562466");

    CacheTimeStamp t3(ms + 2*CacheTimeStamp::S_MS_PER_SEC + 42);
    const char* fmt_sec3 = t3.GetFmtSec();
    string fmt_str3(fmt_sec3);
    string fmt_full_3 = t3.ToString();
    ASSERT_STREQ(fmt_str3.c_str(), "2023_04_21-20_10_35") << "fmt str3:" << fmt_str3 << endl;
    ASSERT_STREQ(fmt_full_3.c_str(), "2023_04_21-20_10_35:562275");
}

