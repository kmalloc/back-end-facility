#ifndef SLOG_FIXEDBUFFER_H_
#define SLOG_FIXEDBUFFER_H_

#include <set>
#include <vector>
#include <stdio.h>
#include <assert.h>
#include <boost/foreach.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/noncopyable.hpp>

namespace slog {

    template<class T>
    class FixedSlab: private boost::noncopyable
    {
        public:
            enum
            {
                ITEM_SIZE = (sizeof(T) < sizeof(void**)? sizeof(void**):sizeof(T))
            };

            // memory returned by malloc & new are guranteed to be aligned for any kind of object.
            explicit FixedSlab(size_t num)
                : m_mem(new char[ITEM_SIZE * num + sizeof(void**)])
                , m_head(reinterpret_cast<void**>(m_mem + ITEM_SIZE * num))
            {
                *m_head = m_mem;
                char* item = reinterpret_cast<char*>(*m_head);

                for (size_t i = 0; i < num - 1; ++i)
                {
                    char* next = item + ITEM_SIZE;
                    *(reinterpret_cast<void**>(item)) = next;
                    item = next;
                }

                *(reinterpret_cast<void**>(item)) = NULL;
            }

            T* AllocElement()
            {
                void* item = *m_head;
                if (!item) return NULL;

                *m_head = *reinterpret_cast<void**>(item);
                return new(item) T();
            }

            void ReleaseElement(T* item)
            {
                item->~T();
                void** i = reinterpret_cast<void**>(item);
                *i = *m_head;
                *m_head = item;
            }

            ~FixedSlab()
            {
                delete[] m_mem;
            }

        private:
            char* const m_mem;
            void** m_head;
    };

    template<size_t N>
    class FixedBuffer: private boost::noncopyable
    {
        public:
            enum { BUFF_SIZE = N };

            FixedBuffer(): m_cur(0) { m_buff[0] = 0; }

            char* Append(const char* data, size_t len)
            {
                char* ret = m_buff + m_cur;

                m_cur += len;
                memcpy(ret, data, len);

                return ret;
            }

            void  Reset() { m_cur = 0; }
            char* Data() { return m_buff; }
            size_t DataLength() const { return m_cur; }
            size_t Available() const { return BUFF_SIZE - m_cur; }
            void SetUsed(size_t size) { m_cur += size; }

        private:
            size_t m_cur;
            char m_buff[N];
    };

    template<size_t N>
    class FixedBufferPool: private boost::noncopyable
    {
        public:
            typedef FixedBuffer<N> BuffType;

            FixedBufferPool(size_t num, size_t inc_gran = 16)
                :m_fix_sz(num), m_inc_gran(inc_gran), m_unrecyled(0)
            {
                assert(num);
                m_pool.reserve(num);
                for (size_t i = 0; i < num; ++i)
                {
                    m_pool.push_back(new BuffType());
                }
            }

            ~FixedBufferPool()
            {
                // no lock
                BOOST_FOREACH(BuffType* b, m_pool)
                {
                    delete b;
                }

                if (m_unrecyled)
                {
                    fprintf(stderr, "%zu buffers unrecyled in FixedBufferPool\n", m_unrecyled);
                }

                if (m_pool.size() > 100 * m_fix_sz)
                {
                    fprintf(stderr,
                            "fixed buffer pool grows "
                            "too fast, cur sz:%zu, fixed:%zu\n",
                            m_pool.size(), m_fix_sz);
                }
            }

            void EnlargePool()
            {
                boost::mutex::scoped_lock lock(m_lock);
                EnlargePoolUnsafe();
            }

            BuffType* GetBuff()
            {
                boost::mutex::scoped_lock lock(m_lock);
                if (m_pool.empty())
                {
                    EnlargePoolUnsafe();
                    if (m_pool.empty()) return NULL;
                }

                // locality, always use newly inserted buffer.
                BuffType* ret = m_pool.back();
                m_pool.pop_back();
                ++m_unrecyled;

                return ret;
            }

            void Recycle(BuffType* buff)
            {
                buff->Reset();
                boost::mutex::scoped_lock lock(m_lock);
                m_pool.push_back(buff);
                --m_unrecyled;
            }

            static void FreeUnrecycledBuff(BuffType* buff)
            {
                delete buff;
            }

            void Shrink(int factor = 1)
            {
                const size_t max_sz = m_fix_sz/factor;

                boost::mutex::scoped_lock lock(m_lock);
                if (m_pool.size() > max_sz)
                {
                    for (size_t i = max_sz; i < m_pool.size(); ++i)
                    {
                        delete m_pool[i];
                    }
                    m_pool.resize(max_sz);
                }
            }

        private:
            void EnlargePoolUnsafe()
            {
                if (m_unrecyled > 1024 * m_fix_sz) return;

                try
                {
                    m_pool.reserve(m_pool.size() + 2 * m_inc_gran);
                    for (size_t i = 0; i < m_inc_gran; ++i)
                    {
                        m_pool.push_back(new BuffType());
                    }
                }
                catch (...)
                {
                }
            }

        private:
            const size_t m_fix_sz;
            const size_t m_inc_gran;

            size_t m_unrecyled;
            boost::mutex m_lock;

            std::vector<BuffType*> m_pool;
    };
}

#endif  // SLOG_FIXEDBUFFER_H_
