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

                for (int i = 0; i < sz; i++)
                {
                    if (vs[i].read) m_read.push_back(vs[i]);
                    
                    if (vs[i].write) m_write.push_back(vs[i]);
                }

                sem_post(&m_sem1);
                sem_wait(&m_sem2);
            }
        }

        const std::vector<PollEvent>& GetRead() const { return m_read; }
        const std::vector<PollEvent>& GetWrite() const { return m_write; }

        void StopRunning() { m_stop = true; }

    private:

        const SocketPoll& m_poll;
        sem_t m_sem1;
        sem_t m_sem2;
        std::vector<PollEvent> m_read;
        std::vector<PollEvent> m_write;
        volatile bool m_stop;
};

struct TestData
{
    int fd;
};


int CreateServerSocket(char* ip, int port)
{
    struct sockaddr_in server_addr;

    int fd_listen = socket(PF_INET, SOCK_STREAM, SOCK_NONBLOCK);

    if (fd_listen == -1) return -1;

    int yes = 1;
    setsockopt(fd_listen, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    bzero(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &(server_addr.sin_addr));
    server_addr.sin_port = htons(port);

    bind(fd_listen, (struct sockaddr*)&server_addr, sizeof(server_addr));

    return fd_listen;
}

TEST(TestWrite, SocketPollTest)
{
    TestData data1, data2, data3;
    sem_t sem1, sem2;
    sem_init(&sem1, 0, 0);
    sem_init(&sem2, 0, 0);

    SocketPoll poll;
    SocketPollListener listener(poll, sem1, sem2);

    // TODO
    
    sem_wait(&sem1);

    // TODO

    sem_post(&sem2);
}


