#ifndef __SOCKET_POLL_H_
#define __SOCKET_POLL_H_

#include <vector>
#include <stdlib.h>

#include "misc/NonCopyable.h"

struct PollEvent
{
    void* data;
    bool  read;
    bool  write;
};

class SocketPoll: public noncopyable
{
    public:

        SocketPoll();
        ~SocketPoll();

        bool AddSocket(int sock, void* data, bool write = false) const;
        bool DeleteSocket(int sock) const;

        bool ChangeSocket(int sock, void* data, bool write = false) const;
        int  WaitAll(std::vector<PollEvent>& ve, size_t max = -1) const;

        bool SetSocketNonBlocking(int fd) const;

    private:

        void Init();
        void Release() const;

        int m_epoll;
};

#endif // __SOCKET_POLL_H_

