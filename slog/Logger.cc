#include "Logger.h"

#include "Likely.h"
#include "AsynLogAppender.h"

#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

static const char g_LogLvlName[slog::Logger::NUM_LOG_LEVEL][8] =
{
    " TRACE ",
    " DEBUG ",
    " INFO  ",
    " WARN  ",
    " ERROR ",
    " FATAL ",
    " ABORT ",
};

class DefaultAppender: public slog::Logger::LogAppender
{
    public:
        virtual void Flush(bool gracefully) { (void)gracefully; }
        virtual size_t Append(const char* msg, size_t len)
        {
            fprintf(stderr, "(default appender)");
            fwrite(msg, sizeof(char), len, stderr);
            return len;
        }
};

typedef boost::shared_ptr<slog::Logger::LogAppender> AppenderSharedPtr;

static char g_app_data[sizeof(AppenderSharedPtr)]
__attribute__((aligned(__alignof__(AppenderSharedPtr))));

static AppenderSharedPtr& g_global_appender = reinterpret_cast<AppenderSharedPtr&>(g_app_data);

static void SignalHandler(int sig_num, siginfo_t* info, void* cnxt)
{
    slog::Logger::FlushPendingLog(false);
    _exit(1);
}

static void InstallSignalHandler()
{
    struct sigaction act;
    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);

    act.sa_flags = SA_SIGINFO;
    act.sa_sigaction = SignalHandler;

    if (sigaction(SIGABRT, &act, NULL) < 0)
        std::cerr << "insall handler for SIGABRT fails";
    if (sigaction(SIGFPE, &act, NULL) < 0)
        std::cerr << "insall handler for SIGFPE fails";
    if (sigaction(SIGILL, &act, NULL) < 0)
        std::cerr << "insall handler for SIGILL fails";
    if (sigaction(SIGSEGV, &act, NULL) < 0)
        std::cerr << "insall handler for SIGSEGV fails";
    if (sigaction(SIGTERM, &act, NULL) < 0)
        std::cerr << "insall handler for SIGTERM fails";
}

namespace slog {

int slogLoggerInitializer::s_counter;
Logger::LogLevel Logger::s_global_level = Logger::LL_INFO;

void slogLoggerInitializer::InitLogger()
{
    new (&g_app_data) AppenderSharedPtr(new DefaultAppender);
}

void slogLoggerInitializer::CleanupLogger()
{
    (&g_global_appender)->~AppenderSharedPtr();
}

static inline slog::LogStream& operator<<(LogStream& ls, const CacheTimeStamp& t)
{
    ls.Append(t.GetFmtSec(), t.FmtSecLength());
    ls.Append(":", 1);
    ls << static_cast<int>(t.TrailingMicroSec());
    return ls;
}

Logger::Logger(CharArray loc, CharArray func, LogLevel lvl)
    : m_level(lvl), m_func(func), m_loc(loc)
{
    CacheTimeStamp t = CacheTimeStamp::Now();
    m_stream << t;
    m_stream.Append(g_LogLvlName[lvl], 7);
}

Logger::~Logger()
{
    m_stream.Append(m_loc.m_data, m_loc.m_sz);
    m_stream.Append(m_func.m_data, m_func.m_sz);
    m_stream.Append("\n", 1);

    g_global_appender->Append(m_stream.Buffer(), m_stream.Length());
    if (UNLIKELY(m_level == LL_ABORT)) abort();
}

static std::string ComputeLogFileName()
{
    std::string home(getenv("HOME"));

    char hostname[215] = {0};
    gethostname(hostname, sizeof(hostname));

    char name[215] = {0};
    readlink("/proc/self/exe", name, sizeof(name));
    char* base_name = basename(name);

    CacheTimeStamp t = CacheTimeStamp::Now();
    std::string date = t.ToString();

    return home + "/" + std::string(base_name)
        + "." + hostname + "." + date + ".log";
}

void Logger::InitLogging(const std::string& file,
        LogLevel level, bool flush_on_crash)
{
    std::string log_file = file;
    if (log_file.empty()) log_file = ComputeLogFileName();

    boost::shared_ptr<LogAppender> app(new AsynLogAppender(log_file));
    InitLogging(app, level, flush_on_crash);
}

void Logger::InitLogging(boost::shared_ptr<LogAppender>& app,
        LogLevel level, bool flush_on_crash)
{
    SetLogLevel(level);
    SetLogAppender(app);
    if (flush_on_crash) InstallSignalHandler();
}

AppenderSharedPtr& Logger::GetLogAppender()
{
    return ::g_global_appender;
}

void Logger::SetLogAppender(const AppenderSharedPtr& app)
{
    ::g_global_appender = app;
}

}  // namespace slog
