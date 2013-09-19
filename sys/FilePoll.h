#ifndef __FILE_POLL_H_
#define __FILE_POLL_H_

#include <vector>
#include <stdlib.h>

struct PollEvent
{
    void* data;
    bool  read;
    bool  write;
};

class FilePoll
{
    public:

        FilePoll();
        ~FilePoll();

        bool AddFile(int sock, void* data, bool write = false) const;
        bool DeleteFile(int sock) const;

        bool ChangeFile(int sock, void* data, bool write = false) const;
        int  WaitAll(std::vector<PollEvent>& ve, size_t max = -1) const;

        bool SetFileNonBlocking(int fd) const;

    private:

        void Init();
        void Release() const;

        int m_epoll;
};

#endif // __FILE_POLL_H_

