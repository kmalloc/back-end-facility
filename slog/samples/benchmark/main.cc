#include "Logger.h"
#include "LogFileAppender.h"
#include "CacheTimeStamp.h"

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#include <stdio.h>
#include <sys/resource.h>

#include <boost/shared_ptr.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/thread.hpp>
#include <boost/thread/mutex.hpp>

#include <gperftools/profiler.h>

using namespace std;
using namespace slog;

static size_t g_thread_num = 4;
static size_t g_per_msg_sz = 0;
static const size_t g_log_num = 20 * 1000 * 1000; // 30 milion
static string g_dummy_log("xxyyzzyyysjdlffflsdwweefdsgg45345");

static FILE* g_fprintf_fp = NULL;
static bool  g_should_clear_file = true;
static const string g_loc_file = string(getenv("HOME")) + "/as.log";
static const string g_nfs_file = string(getenv("HOME")) + "/sync_log.log";

class DummyAppender: public Logger::LogAppender
{
    public:
        DummyAppender(): m_total(0) {}

        virtual void Flush(bool gracefully) { (void)gracefully; }

        virtual size_t Append(const char* msg, size_t len)
        {
            (void)msg;
            m_total += len;
            return len;
        }

        size_t GetTotal() const { return m_total; }

    private:
        size_t m_total;
};

#define FMT_OUT(type, tid, log, id) "logging benchmark, type:" << type \
        << ", thread " << tid << ", msg:" << log << ", num: " << id

void thread_func(int tid, const char* type, int mode)
{
    if (mode == 1)
    {
        for (size_t i = 0; i < g_log_num / g_thread_num; ++i)
        {
            SLOG_INFO << FMT_OUT(type, tid, g_dummy_log, i);
        }
    }
    else
    {
        const char* msg = g_dummy_log.c_str();
        for (size_t i = 0; i < g_log_num / g_thread_num; ++i)
        {
            CacheTimeStamp t = CacheTimeStamp::Now();
            g_per_msg_sz = fprintf(g_fprintf_fp, "%s:%lld INFO logging benchmark, type:%s, "
                    "thread %d, msg: %s, num: %zu\n", t.GetFmtSec(),
                    t.TrailingMicroSec(), type, tid, msg, i);
        }
    }
}

void ClearFile(const std::string& file)
{
    ofstream fout(file.c_str());
    fout << "";
}

void CalcMsgSz()
{
    boost::shared_ptr<Logger::LogAppender> app(new DummyAppender);

    Logger::InitLogging(app, Logger::LL_DEBUG);

    SLOG_INFO << FMT_OUT("dummy logging to strstream", 0, g_dummy_log, 0);
    g_per_msg_sz = dynamic_cast<DummyAppender*>(app.get())->GetTotal();
}

void test_log_file(const std::string& file)
{
    {
        LogFile log(file);

        CacheTimeStamp ts = CacheTimeStamp::Now();
        for (size_t i = 0; i < g_log_num; ++i)
        {
            log.Append(g_dummy_log.c_str(), g_dummy_log.size());
        }
        CacheTimeStamp te = CacheTimeStamp::Now();

        double sec_used = te - ts;
        double data_sz = g_log_num * g_dummy_log.size() / (1024*1024);
        cout << "summary for LogFile to " << file << ", time used:" << sec_used
            << " secs, data:" << data_sz << " MB, data rate:"
            << data_sz/sec_used << " MB/S " << endl;
    }

    ClearFile(file);
}

void bench(const char* type, int mode = 1, bool should_clear_file = true)
{
    CacheTimeStamp ts = CacheTimeStamp::Now();

    if (g_thread_num > 1)
    {
        vector<boost::shared_ptr<boost::thread> > tv;
        tv.reserve(g_thread_num);

        for (size_t i = 0; i < g_thread_num; ++i)
        {
            boost::shared_ptr<boost::thread> t(new boost::thread(thread_func, i, type, mode));
            tv.push_back(t);
        }

        for (size_t i = 0; i < g_thread_num; ++i)
        {
            tv[i]->join();
        }
    }
    else
    {
        thread_func(0, type, mode);
    }

    CacheTimeStamp te = CacheTimeStamp::Now();

    size_t total = 0;
    double sec_used = te - ts;
    total = g_log_num * g_per_msg_sz / (1024 * 1024);

    cout << "summary for " << type << ": " << sec_used << " seconds, "
         << total << " mega bytes, " << g_log_num/sec_used << " msg/s, "
         << g_per_msg_sz << " bytes/msg, "
         << total / sec_used << " MB/s" << std::endl;

    Logger::GetLogAppender()->Flush(true);
    sleep(1);

    if (mode == 0)
    {
        CalcMsgSz();
        fclose(g_fprintf_fp);
    }

    if (should_clear_file && g_should_clear_file)
    {
        ClearFile(g_loc_file);
        ClearFile(g_nfs_file);
    }
}

static inline ostream& operator<<(ostream& ls, const CacheTimeStamp& t)
{
    ls << t.GetFmtSec() << ":" << t.TrailingMicroSec();
    return ls;
}

void RawStreamLogging(const char* type, ostream& os)
{
    CacheTimeStamp ts = CacheTimeStamp::Now();
    for (size_t i = 0; i < g_log_num; ++i)
    {
        CacheTimeStamp t = CacheTimeStamp::Now();
        os << t << " " << FMT_OUT(type, 0, g_dummy_log, i)
            << "-" << __BASE_FILE__ << ":" << __LINE__
            << ":" << __func__ << "\n";
    }
    CacheTimeStamp te = CacheTimeStamp::Now();

    double sec_used = te - ts;
    size_t total = g_log_num * g_per_msg_sz / (1024 * 1024);

    std::cout << std::endl << "summary for " << type << ", "
        << sec_used << " seconds, " << g_log_num/sec_used << " msg/s, "
        << g_per_msg_sz << " bytes/msg, "
        << total << " mega bytes, " << total / sec_used << " MB/s" << std::endl;

    cout  << "end testing for " << type << endl << endl;
}

int main(int argc, char* argv[])
{
    size_t mode = 1 & (1 << 1) & (1 << 2) & (1 << 3);

    if (argc >= 2) g_thread_num = atoi(argv[1]);
    if (argc >= 3) mode = atoi(argv[2]);
    if (argc >= 4) g_should_clear_file = atoi(argv[3]);

    CalcMsgSz();
    fprintf(stderr, "%zu threads for benchmark testing.\n", g_thread_num);

    if (!mode) RawStreamLogging("cout bm", cout);

    boost::shared_ptr<Logger::LogAppender> app;
    boost::shared_ptr<Logger::LogAppender> app2;

    if (mode & 1)
    {
        ofstream fout1(g_loc_file.c_str());
        RawStreamLogging("ofstream logging to local file", fout1);
        ofstream fout2(g_nfs_file.c_str());
        RawStreamLogging("ofstream logging to nfs file", fout2);

        ClearFile(g_loc_file);
        ClearFile(g_nfs_file);
    }

    if (mode & 2)
    {
        app.reset(new SynLogAppender("/dev/null"));
        Logger::InitLogging(app, Logger::LL_DEBUG);
        bench("lock sync logging to /dev/null");

        app.reset(new SynLogAppender(g_loc_file));
        Logger::InitLogging(app, Logger::LL_DEBUG);
        bench("lock sync logging to local file");

        app.reset(new SynLogAppender(g_nfs_file));
        Logger::InitLogging(app, Logger::LL_DEBUG);
        bench("lock sync logging to NFS file");

        cout  << "end testing for lock sync logging" << endl << endl;
    }

    if ((mode & 4) && g_thread_num <= 1)
    {
        app.reset(new LogFile("/dev/null"));
        Logger::InitLogging(app, Logger::LL_DEBUG);
        bench("no lock sync logging to /dev/null");

        app.reset(new LogFile(g_loc_file));
        Logger::InitLogging(app, Logger::LL_DEBUG);
        bench("no lock sync logging to local file");

        app.reset(new LogFile(g_nfs_file));
        Logger::InitLogging(app, Logger::LL_DEBUG);
        bench("no lock sync logging to NFS file");

        cout  << "end testing for nolock sync logging" << endl << endl;
    }

    if (mode & 8)
    {
        // Logger::InitLogging("/dev/null");
        // bench("async logging to /dev/null");

        ProfilerStart("slog_prof.out");

        Logger::InitLogging(g_loc_file);
        bench("async logging to local file");

        // Logger::InitLogging(g_nfs_file);
        // bench("async logging to NFS file");

        cout << "end testing for async log" << endl << endl;
        ProfilerStop();
    }

    if (mode & 16)
    {
        g_fprintf_fp = fopen(g_loc_file.c_str(), "w");
        assert(g_fprintf_fp);
        bench("fprintf logging to local file", 0);

        g_fprintf_fp = fopen(g_nfs_file.c_str(), "w");
        assert(g_fprintf_fp);
        bench("fprintf logging to nfs file", 0, false);
    }

    if (mode & 32)
    {
        test_log_file(g_loc_file);
        test_log_file(g_nfs_file);
    }

    return 0;
}

