#include "gtest.h"

#include "LockFreeBuffer.h"
#include "lock-free-list.h"
#include "thread/ITask.h"
#include "thread/thread.h"

static const int gs_buffers = 1024;
static const int gs_size    = 512;
static const char gs_candy[] = "\x23\x23\xcd\xdc\xff\x4f\x09\xcc";
static const int gs_candy_sz = sizeof(gs_candy);


static inline void SetupCandy(void* buffer)
{
    memcpy(buffer, gs_candy, gs_candy_sz);
    memcpy(buffer + gs_candy_sz, gs_candy, gs_candy_sz);
}

static inline bool VerifyCandy(void* buffer)
{
    return memcmp(buffer, gs_candy, gs_candy_sz) == 0
        && memcmp((char*)buffer + gs_candy_sz, gs_candy, gs_candy_sz) == 0;
}

class LockFreeBufferReleaseThread: public ThreadBase
{
    public:

        LockFreeBufferReleaseThread(LockFreeBuffer& alloc)
            :m_stop(false)
            ,m_queue(3072)
            ,m_alloc(alloc)
            
        {
        }

        bool PushBuffer(void* buff)
        {
            return m_queue.Push(buff);
        }

        virtual void Run()
        {
            void* buffer = NULL;
            while (!m_stop)
            {
               bool ret = m_queue.Pop(&buffer); 

               if (ret) 
               {
                   EXPECT_TRUE(VerifyCandy(buffer));
                   m_alloc.ReleaseBuffer(buffer);
                   continue;
               }

               sched_yield();
            }
        }

        void Stop() { m_stop = true; }

    private:

        bool m_stop;
        LockFreeBuffer& m_alloc;
        LockFreeListQueue m_queue;
};


class LockFreeBufferConsumerThread: public ThreadBase
{
    public:

        LockFreeBufferConsumerThread(LockFreeBuffer& alloc,
                LockFreeBufferReleaseThread& storage)
            :m_alloc(alloc)
            ,m_storage(storage)
        {
        }
        
        void Stop() { m_stop = true; }

        virtual void Run()
        {
            void* buffer = NULL;
            while (!m_stop)
            {
                buffer = m_alloc.AllocBuffer();

                if (buffer == NULL)
                {
                    sched_yield(); 
                    continue;
                }

                SetupCandy(buffer);
                if (!m_storage.PushBuffer(buffer))
                {
                    EXPECT_TRUE(VerifyCandy(buffer));
                    m_alloc.ReleaseBuffer(buffer);
                    sched_yield();
                }
            }
        }

    private:

        bool m_stop;
        LockFreeBuffer& m_alloc;
        LockFreeBufferReleaseThread& m_storage;
};

TEST(LockFreeBufferTest, LockFreeBufferTest)
{
    LockFreeBuffer m_alloc(1024, 2 * gs_candy_sz);

    LockFreeBufferReleaseThread m_release(m_alloc);

    LockFreeBufferConsumerThread m_consumer1(m_alloc, m_release);
    LockFreeBufferConsumerThread m_consumer2(m_alloc, m_release);
    LockFreeBufferConsumerThread m_consumer3(m_alloc, m_release);

    m_release.Start();
    m_consumer1.Start();
    m_consumer2.Start();
    m_consumer3.Start();


    sleep(12);

    m_consumer1.Stop();
    m_consumer2.Stop();
    m_consumer3.Stop();

    m_consumer1.Join();
    m_consumer2.Join();
    m_consumer3.Join();

    m_release.Stop();
    m_release.Join();
}

