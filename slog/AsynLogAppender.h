#ifndef SLOG_ASYNLOGAPPENDER_H_
#define SLOG_ASYNLOGAPPENDER_H_

#include "FixedBuffer.h"
#include "LogFileAppender.h"

#include <string>
#include <vector>
#include <pthread.h>
// #include <boost/atomic.hpp> // not until boost.1.57.0
#include <boost/scoped_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/condition_variable.hpp>

namespace slog {

    class AsynLogAppender: public Logger::LogAppender
    {
        public:
            // file: log file path.
            // interval: flush pending log every 'interval' seconds
            // flush_cnt: if number of pending log is > flush_cnt, then do flush.
            explicit AsynLogAppender(const std::string& file,
                    size_t interval = 1, size_t flush_cnt = 512);
            virtual ~AsynLogAppender() { Stop(); }

            virtual void Flush(bool gracefully);
            virtual size_t Append(const char* msg, size_t len);

        public:
            static const int S_BUFF_SIZE = 256 * 1024;
            typedef FixedBufferPool<S_BUFF_SIZE> BuffPoolType;
            typedef FixedBufferPool<S_BUFF_SIZE>::BuffType BuffType;

            struct LogMsg
            {
                // if len == 0, user of this msg should delete 'data';
                size_t len;
                char* data;
            };

            struct PerThreadItem
            {
                ~PerThreadItem();

                BuffType* buff;
                boost::weak_ptr<AsynLogAppender> app;
            };

        private:
            void Stop();
            void ShrinkMemoryUsageUnlock();
            static void LoggingThreadFunc(AsynLogAppender* app);
            inline void AddToWriter(char* addr, size_t len, char* buff);

        private:
            bool m_running; // better to be atomic.
            size_t m_curPendingMsg;
            const size_t m_interval;
            const size_t m_flush_cnt;

            boost::mutex m_lock;
            boost::condition_variable m_ready_sig;

            LogFile m_log_file;
            BuffPoolType m_pool;

            std::vector<LogMsg> m_pending_buff1;
            std::vector<LogMsg> m_pending_buff2;

            std::vector<LogMsg>* m_pb1;
            std::vector<LogMsg>* m_pb2;

            boost::scoped_ptr<boost::thread> m_thread;
    };
}

#endif  // SLOG_ASYNLOGAPPENDER_H_
