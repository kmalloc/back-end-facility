#include "FilePoll.h"

#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>


FilePoll::FilePoll()
    :m_epoll(-1)
{
    Init();
}

void FilePoll::Init()
{
    m_epoll = epoll_create(1024);
    assert(m_epoll == -1);
}

FilePoll::~FilePoll()
{
    close(m_epoll);
}

bool FilePoll::AddFile(int file, void* data, bool write) const
{
    struct epoll_event ev;
    ev.events = EPOLLIN | (write? EPOLLOUT : 0);
    ev.data.ptr = data;

    return (epoll_ctl(m_epoll, EPOLL_CTL_ADD, file, &ev) != -1); 
}

bool FilePoll::DeleteFile(int file) const
{
    return (epoll_ctl(m_epoll, EPOLL_CTL_DEL, file, NULL) != -1);
}

bool FilePoll::ChangeFile(int file, void* data, bool write) const
{
    struct epoll_event ev;
    ev.events = EPOLLIN | (write? EPOLLOUT : 0);
    ev.data.ptr = data;

    return (epoll_ctl(m_epoll, EPOLL_CTL_MOD, file, &ev) != -1);
}

int FilePoll::WaitAll(std::vector<PollEvent>& ve, size_t max) const
{
    PollEvent event;
    // variant array
    struct epoll_event ev[max];
    int n = epoll_wait(m_epoll, ev, max, -1);

    for (int i = 0; i < n; ++i)
    {
        unsigned flag = ev[i].events;
        event.data  = ev[i].data.ptr;
        event.write = (flag & EPOLLOUT) != 0;
        event.read  = (flag & EPOLLIN) != 0;

        ve.push_back(event);
    }

    return n;
}

bool FilePoll::SetFileNonBlocking(int fd) const
{
    int flag = fcntl(fd, F_GETFL, 0);

    if (-1 == flag) return false;

    return fcntl(fd, F_SETFL, flag | O_NONBLOCK) != -1;
}

