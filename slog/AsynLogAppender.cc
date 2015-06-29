#include "AsynLogAppender.h"
#include "Likely.h"

#include <assert.h>
#include <boost/thread/tss.hpp>
#include <boost/shared_ptr.hpp>

namespace slog {

static const size_t g_msg_queue_sz = 4 * 4096;

static boost::thread_specific_ptr<AsynLogAppender::PerThreadItem> tl_buff_info;

AsynLogAppender::PerThreadItem::~PerThreadItem()
{
    boost::shared_ptr<AsynLogAppender> pt = app.lock();
    if (!pt)
    {
        if (!buff) BuffPoolType::FreeUnrecycledBuff(buff);

        return;
    }

    // this function is called right before current thread exits,
    // no more logging operations are allowed after pt->AddToWriter()
    // otherwise there will be memory leak: logging will alloc buffer.
    // and after thread exit, no way to free that allocated buffer.
    pt->AddToWriter(NULL, 0, reinterpret_cast<char*>(buff));
}

AsynLogAppender::AsynLogAppender(const std::string& file, size_t interval, size_t flush_cnt)
    : m_running(true), m_curPendingMsg(0), m_interval(interval)
    , m_flush_cnt(flush_cnt), m_log_file(file), m_pool(32)
    , m_pb1(&m_pending_buff1), m_pb2(&m_pending_buff2)
    , m_thread(new boost::thread(&AsynLogAppender::LoggingThreadFunc, this))
{
    BOOST_STATIC_ASSERT(S_BUFF_SIZE > LogStreamFixed::S_MAX_BUFF_SZ);
    BOOST_STATIC_ASSERT(LogFile::S_DEF_BUFF_SZ >
            static_cast<size_t>(LogStreamFixed::S_MAX_BUFF_SZ));

    m_pending_buff1.reserve(g_msg_queue_sz);
    m_pending_buff2.reserve(m_pending_buff1.capacity());
}

void AsynLogAppender::Stop()
{
    // no logging here, since it will call from destructor of this appender
    {
        boost::mutex::scoped_lock lock(m_lock);
        m_running = false;
        m_ready_sig.notify_one();
    }

    m_thread->join();
}

void AsynLogAppender::Flush(bool gracefully)
{
    if (gracefully)
    {
        m_ready_sig.notify_one();
        return;
    }

    for (size_t i = 0; i < m_pending_buff2.size(); ++i)
    {
        if (!m_pending_buff2[i].len) continue;

        m_log_file.AppendRaw("(f2)", 4);
        m_log_file.AppendRaw(m_pending_buff2[i].data, m_pending_buff2[i].len);
    }

    for (size_t i = 0; i < m_pending_buff1.size(); ++i)
    {
        if (!m_pending_buff1[i].len) continue;

        m_log_file.AppendRaw("(f)", 3);
        m_log_file.AppendRaw(m_pending_buff1[i].data, m_pending_buff1[i].len);
    }
}

size_t AsynLogAppender::Append(const char* msg, size_t len)
{
    assert(len && msg);

    if (UNLIKELY(!tl_buff_info.get()))
    {
        tl_buff_info.reset(new PerThreadItem);

        tl_buff_info->buff = m_pool.GetBuff();
        tl_buff_info->app  =
            boost::static_pointer_cast<AsynLogAppender>(shared_from_this());
    }

    BuffType* buff = tl_buff_info->buff;

    if (UNLIKELY(!buff) || buff->Available() < len)
    {
        if (LIKELY(buff)) AddToWriter(NULL, 0, reinterpret_cast<char*>(buff));

        tl_buff_info->buff = buff = m_pool.GetBuff();
        if (UNLIKELY(!buff))
        {
            // out of memory, drop message.
            fprintf(stderr, "out of memory for asyn logging,"
                    " dropping msg: %s\n", msg);
            return 0;
        }
    }

    char* addr = buff->Append(msg, len);
    AddToWriter(addr, len, reinterpret_cast<char*>(buff));
    return len;
}

void AsynLogAppender::AddToWriter(char* addr, size_t len, char* buff)
{
    if (!addr && !len)
    {
        assert(buff);
        addr = buff;
    }

    LogMsg msg = {len, addr};

    {
        boost::mutex::scoped_lock lock(m_lock);

        assert(m_running);
        m_pb1->push_back(msg);
        ++m_curPendingMsg;
        if (m_curPendingMsg < m_flush_cnt) return;
    }

    m_ready_sig.notify_one();
}

void AsynLogAppender::LoggingThreadFunc(AsynLogAppender* this_)
{
    // logging in worker thread is not allowed!!

    std::vector<LogMsg>* vp = NULL;
    const boost::posix_time::milliseconds timeout(this_->m_interval * 1000);

    while (true)
    {
        {
            int idle = 0;
            boost::unique_lock<boost::mutex> lock(this_->m_lock);
            while (this_->m_running)
            {
                this_->m_ready_sig.timed_wait(lock, timeout);
                if (!this_->m_pb1->empty()) break;

                if (++idle != 8) continue;

                this_->ShrinkMemoryUsageUnlock();
            }

            if (!this_->m_running && this_->m_pb1->empty()) break;

            vp = this_->m_pb1;
            this_->m_curPendingMsg = 0;
            std::swap(this_->m_pb1, this_->m_pb2);
        }

        for (size_t i = 0; i < vp->size(); ++i)
        {
            if (!(*vp)[i].len)
            {
                this_->m_pool.Recycle(reinterpret_cast<BuffType*>((*vp)[i].data));
            }
            else
            {
                this_->m_log_file.Append((*vp)[i].data, (*vp)[i].len);
            }
        }

        this_->m_log_file.Flush(true);
        vp->clear();
    }
}

void AsynLogAppender::ShrinkMemoryUsageUnlock()
{
    m_pool.Shrink();

    if (m_pending_buff1.capacity() > g_msg_queue_sz)
    {
        std::vector<LogMsg> tmp;
        tmp.reserve(g_msg_queue_sz);
        m_pending_buff1.swap(tmp);
    }

    if (m_pending_buff2.capacity() > g_msg_queue_sz)
    {
        std::vector<LogMsg> tmp;
        tmp.reserve(g_msg_queue_sz);
        m_pending_buff2.swap(tmp);
    }
}

}

