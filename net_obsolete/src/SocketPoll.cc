#include "SocketPoll.h"

#include <sys/epoll.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>


SocketPoll::SocketPoll()
    :epoll_(-1)
{
    Init();
}

void SocketPoll::Init()
{
    epoll_ = epoll_create(1024);
    assert(epoll_ != -1);
}

SocketPoll::~SocketPoll()
{
    Release();
}

void SocketPoll::Release() const
{
    close(epoll_);
}

bool SocketPoll::AddSocket(int file, void* data, bool write) const
{
    struct epoll_event ev;
    ev.events = EPOLLIN | (write? EPOLLOUT : 0);
    ev.data.ptr = data;

    int ret = epoll_ctl(epoll_, EPOLL_CTL_ADD, file, &ev);
    return (ret != -1);
}

bool SocketPoll::RemoveSocket(int file) const
{
    return (epoll_ctl(epoll_, EPOLL_CTL_DEL, file, NULL) != -1);
}

bool SocketPoll::ModifySocket(int file, void* data, bool write) const
{
    struct epoll_event ev;
    ev.events = EPOLLIN | (write? EPOLLOUT : 0);
    ev.data.ptr = data;

    return (epoll_ctl(epoll_, EPOLL_CTL_MOD, file, &ev) != -1);
}

int SocketPoll::WaitAll(PollEvent* ve, size_t max) const
{
    PollEvent event;
    // variant array
    struct epoll_event ev[max];
    int n = epoll_wait(epoll_, ev, max, -1);

    for (int i = 0; i < n; ++i)
    {
        unsigned flag = ev[i].events;
        event.data  = ev[i].data.ptr;
        event.write = ((flag & EPOLLOUT) != 0);
        event.read  = ((flag & EPOLLIN) != 0);

        ve[i] = event;
    }

    return n;
}

bool SocketPoll::SetSocketNonBlocking(int fd)
{
    int flag = fcntl(fd, F_GETFL, 0);

    if (-1 == flag) return false;

    return fcntl(fd, F_SETFL, flag | O_NONBLOCK) != -1;
}

