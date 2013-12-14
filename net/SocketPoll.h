#ifndef __SOCKET_POLL_H_
#define __SOCKET_POLL_H_

#include <vector>
#include <stdlib.h>

#include "misc/NonCopyable.h"

/*
 * simple wrapper of epoll.
 * technically, It should be named as FilePoll, because other file descriptor is not restricted,
 * just leave it as it is for the moment. 
 */

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
        bool RemoveSocket(int sock) const;

        bool ModifySocket(int sock, void* data, bool write = false) const;
        int  WaitAll(PollEvent* ve, size_t max = -1) const;

        static bool SetSocketNonBlocking(int fd);

    private:

        void Init();
        void Release() const;

        int epoll_;
};

#endif // __SOCKET_POLL_H_

