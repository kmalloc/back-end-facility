#ifndef SLOG_LOGGER_H_
#define SLOG_LOGGER_H_

#include "LogStream.h"
#include "CacheTimeStamp.h"

#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/enable_shared_from_this.hpp>

namespace slog {

class Logger
{
    public:
        enum LogLevel
        {
            LL_TRACE,
            LL_DEBUG,
            LL_INFO,
            LL_WARN,
            LL_ERROR,
            LL_FATAL,
            LL_ABORT,
            NUM_LOG_LEVEL,
        };

        class LogAppender: private boost::noncopyable,
                           public boost::enable_shared_from_this<LogAppender>
        {
            public:
                virtual ~LogAppender() {}

                virtual void Flush(bool gracefully) = 0;
                virtual size_t Append(const char* data, size_t len) = 0;
        };

        class CharArray
        {
            public:
                CharArray(): m_sz(0), m_data(NULL) {}

                template<size_t N>
                CharArray(const char(&arr)[N])
                    : m_sz(N - 1), m_data(arr)
                {
                }
                const size_t m_sz;
                const char* m_data;
        };

        Logger(CharArray loc,  CharArray func, LogLevel);
        ~Logger();

        LogStream& GetStream() { return m_stream; }

        static LogLevel GetLogLevel() { return s_global_level; }
        // change current log level, not thread safe.
        static void SetLogLevel(LogLevel l) { s_global_level = l; }

        static boost::shared_ptr<LogAppender>& GetLogAppender();
        // change log appender, not thread safe.
        static void SetLogAppender(const boost::shared_ptr<LogAppender>& app);

        static void FlushPendingLog(bool gracefully = true)
        {
            GetLogAppender()->Flush(gracefully);
        }

        static void InitLogging(const std::string& log_file = "",
                LogLevel level = LL_INFO, bool flush_on_crash = true);

        static void InitLogging(boost::shared_ptr<LogAppender>& app,
                LogLevel level = LL_INFO, bool flush_on_crash = true);

    private:
        int m_level;
        LogStreamFixed m_stream;
        const CharArray m_func, m_loc;
        static LogLevel s_global_level;
};

// Nifty Counter
class slogLoggerInitializer
{
    public:
        slogLoggerInitializer()
        {
            if (s_counter++ == 0) InitLogger();
        }

        ~slogLoggerInitializer()
        {
            if (--s_counter == 0) CleanupLogger();
        }

    private:
        void InitLogger();
        void CleanupLogger();

    private:
        static int s_counter;
};

static slogLoggerInitializer s_slog_logger_init;

}  // namespace slog

#define SLOG_STRINGIFY(x) #x
#define SLOG_TOSTRING(x) SLOG_STRINGIFY(x)
#define SLOG_LOC_INFO "-"__BASE_FILE__":"SLOG_TOSTRING(__LINE__)":"

#define TRY_LOG(level) \
    if ((level) >= slog::Logger::GetLogLevel()) \
        slog::Logger(SLOG_LOC_INFO, __func__, (level)).GetStream()

#define SLOG_TRACE TRY_LOG(slog::Logger::LL_TRACE)
#define SLOG_DEBUG TRY_LOG(slog::Logger::LL_DEBUG)
#define SLOG_INFO TRY_LOG(slog::Logger::LL_INFO)
#define SLOG_WARN TRY_LOG(slog::Logger::LL_WARN)
#define SLOG_ERROR TRY_LOG(slog::Logger::LL_ERROR)
#define SLOG_FATAL TRY_LOG(slog::Logger::LL_FATAL)
#define SLOG_ABORT TRY_LOG(slog::Logger::LL_ABORT)

#define SLOG_CHECK(condition) \
    if (!(condition)) \
        SLOG_ABORT << "condition:" << #condition << " failed! "


#ifdef SLOG_TEST_GLOBAL_INIT
// for testing global value initialization
static int slog_dummy_func()
{
    static int s_i = 42;
    SLOG_INFO << __BASE_FILE__", logging from initializing global value:" << s_i;
    return s_i++;
}

static int g_slog_dummy_int1 = slog_dummy_func();
static int g_slog_dummy_int2 = slog_dummy_func();
static int g_slog_dummy_int3 = slog_dummy_func();
static int g_slog_dummy_int4 = slog_dummy_func();
#endif

#endif  // SLOG_LOGGER_H_
