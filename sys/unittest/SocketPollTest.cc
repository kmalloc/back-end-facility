#include <gtest.h>

#include "thread/Thread.h"
#include "sys/SocketPoll.h"

#include <vector>
#include <stdio.h>
#include <semaphore.h>

#include <fcntl.h>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h> //PIPE_BUF

typedef std::vector<PollEvent> VPE;

class SocketPollListener: public ThreadBase
{
    public:

        SocketPollListener(const SocketPoll& poll, sem_t& sem1, sem_t& sem2)
            : m_poll(poll)
            , m_sem1(sem1)
            , m_sem2(sem2)
            , m_stop(false)
        {
        }

        virtual void Run()
        {
            std::vector<PollEvent> vs;
            vs.reserve(64);

            while (!m_stop)
            {
                int sz = m_poll.WaitAll(vs, 1024);

                m_read.clear();
                m_write.clear();

                m_events = sz;

                EXPECT_EQ(sz, vs.size());

                for (int i = 0; i < sz; i++)
                {
                    if (vs[i].read) m_read.push_back(vs[i]);

                    if (vs[i].write) m_write.push_back(vs[i]);
                }

                vs.clear();

                sem_post(&m_sem1);
                sem_wait(&m_sem2);
            }
        }

        const VPE& GetRead() const { return m_read; }
        const VPE& GetWrite() const { return m_write; }

        void StopRunning() { m_stop = true; }
        int GetEventNum() { return m_events; }

    private:

        const SocketPoll& m_poll;
        sem_t& m_sem1;
        sem_t& m_sem2;
        std::vector<PollEvent> m_read;
        std::vector<PollEvent> m_write;
        volatile int m_events;
        volatile bool m_stop;
};

struct TestData
{
    int fd;
};

static inline int convert(const PollEvent& event)
{
    return ((TestData*)(event.data))->fd;
}

TEST(TestWrite, SocketPollTest)
{
    TestData data1, data2, data3;
    sem_t sem1, sem2;
    sem_init(&sem1, 0, 0);
    sem_init(&sem2, 0, 0);

    SocketPoll poll;
    SocketPollListener listener(poll, sem1, sem2);

    listener.Start();

    int fd_reads[3][2];

    pipe(fd_reads[0]);
    pipe(fd_reads[1]);
    pipe(fd_reads[2]);

    data1.fd = fd_reads[0][0];
    data2.fd = fd_reads[1][0];
    data3.fd = fd_reads[2][0];

    poll.SetSocketNonBlocking(fd_reads[0][0]);
    poll.SetSocketNonBlocking(fd_reads[1][0]);
    poll.SetSocketNonBlocking(fd_reads[2][0]);
    poll.AddSocket(fd_reads[0][0], &data1);
    poll.AddSocket(fd_reads[1][0], &data2);
    poll.AddSocket(fd_reads[2][0], &data3);

    // write to one pile
    write(fd_reads[0][1], "abc", 4);

    sem_wait(&sem1);

    const VPE& reads  = listener.GetRead();
    const VPE& writes = listener.GetWrite();

    ASSERT_EQ(1, reads.size());
    EXPECT_EQ(fd_reads[0][0], convert(reads[0]));
    EXPECT_TRUE(writes.empty());

    // write to 3 pipe
    write(fd_reads[0][1], "abc", 4);
    write(fd_reads[1][1], "abc", 4);
    write(fd_reads[2][1], "abc", 4);

    sem_post(&sem2);
    sem_wait(&sem1);

    const VPE& reads2  = listener.GetRead();
    const VPE& writes2 = listener.GetWrite();

    ASSERT_EQ(3, reads2.size());
    EXPECT_TRUE(convert(reads2[0]) != convert(reads2[1]));
    EXPECT_TRUE(convert(reads2[0]) != convert(reads2[2]));
    EXPECT_TRUE(convert(reads2[1]) != convert(reads2[2]));

    EXPECT_TRUE(convert(reads2[0]) == fd_reads[0][0] || convert(reads2[0]) == fd_reads[1][0] || convert(reads2[0]) == fd_reads[2][0]);
    EXPECT_TRUE(convert(reads2[1]) == fd_reads[0][0] || convert(reads2[1]) == fd_reads[1][0] || convert(reads2[1]) == fd_reads[2][0]);
    EXPECT_TRUE(convert(reads2[2]) == fd_reads[0][0] || convert(reads2[2]) == fd_reads[1][0] || convert(reads2[2]) == fd_reads[2][0]);

    EXPECT_TRUE(writes2.empty());

    // test write
    TestData data4, data5, data6;
    data4.fd = fd_reads[0][1];
    data5.fd = fd_reads[1][1];
    data6.fd = fd_reads[2][1];

    const bool enable_write = true;
    poll.AddSocket(fd_reads[0][1], &data4, enable_write);
    poll.AddSocket(fd_reads[1][1], &data5, enable_write);
    poll.AddSocket(fd_reads[2][1], &data6, enable_write);

    poll.SetSocketNonBlocking(fd_reads[0][1]);
    poll.SetSocketNonBlocking(fd_reads[1][1]);
    poll.SetSocketNonBlocking(fd_reads[2][1]);

    const int big = 200 * PIPE_BUF + 2;
    
    char* bigbuf = new char[big];
    int wz1 = write(fd_reads[0][1], bigbuf, big);
    int wz2 = write(fd_reads[1][1], bigbuf, big);
    int wz3 = write(fd_reads[2][1], bigbuf, big);

    int num_write = 0;
    if (wz1 < big) num_write++;
    if (wz2 < big) num_write++;
    if (wz3 < big) num_write++;

    sem_post(&sem2);
    sem_wait(&sem1);

    const VPE& reads3 = listener.GetRead();
    const VPE& writes3 = listener.GetWrite();

    EXPECT_EQ(3, listener.GetEventNum());
    ASSERT_EQ(3, reads3.size()) << "buf sz:" << big << std::endl;
    ASSERT_EQ(0, writes3.size()) << "buf sz:" << big << std::endl;

    EXPECT_TRUE(convert(reads3[0]) != convert(reads3[1]));
    EXPECT_TRUE(convert(reads3[0]) != convert(reads3[2]));
    EXPECT_TRUE(convert(reads3[1]) != convert(reads3[2]));

    EXPECT_TRUE(convert(reads3[0]) == fd_reads[0][0] || convert(reads3[0]) == fd_reads[1][0] || convert(reads3[0]) == fd_reads[2][0]);
    EXPECT_TRUE(convert(reads3[1]) == fd_reads[0][0] || convert(reads3[1]) == fd_reads[1][0] || convert(reads3[1]) == fd_reads[2][0]);
    EXPECT_TRUE(convert(reads3[2]) == fd_reads[0][0] || convert(reads3[2]) == fd_reads[1][0] || convert(reads3[2]) == fd_reads[2][0]);

    read(fd_reads[0][0], bigbuf, wz1);
    read(fd_reads[1][0], bigbuf, wz2);
    read(fd_reads[2][0], bigbuf, wz3);

    sem_post(&sem2);

    if (num_write)
    {
        std::cout << "test write poll" << std::endl;

        sem_wait(&sem1);

        const VPE& writes4 = listener.GetWrite();

        ASSERT_EQ(num_write, writes4.size());

        EXPECT_TRUE(convert(writes3[0]) != convert(writes3[1]));
        EXPECT_TRUE(convert(writes3[0]) != convert(writes3[2]));
        EXPECT_TRUE(convert(writes3[1]) != convert(writes3[2]));

        EXPECT_TRUE(convert(writes3[0]) == fd_reads[0][1] || convert(writes3[0]) == fd_reads[1][1] || convert(writes3[0]) == fd_reads[2][1]);
        EXPECT_TRUE(convert(writes3[1]) == fd_reads[0][1] || convert(writes3[1]) == fd_reads[1][1] || convert(writes3[1]) == fd_reads[2][1]);
        EXPECT_TRUE(convert(writes3[2]) == fd_reads[0][1] || convert(writes3[2]) == fd_reads[1][1] || convert(writes3[2]) == fd_reads[2][1]);
    }

    listener.StopRunning();
    sem_post(&sem2);

    delete [] bigbuf;
}


