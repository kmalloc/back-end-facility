#include "SocketServer.h"
#include "SocketPoll.h"

#include "sys/Log.h"
#include "sys/Defs.h"
#include "sys/AtomicOps.h"
#include "thread/Thread.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <queue>

typedef int (* SocketPredicateProc)(int, struct addrinfo*);

enum SocketStatus
{
    SS_INVALID,
    SS_LISTENING,
    SS_CONNECTED,
    SS_CONNECTING,
    SS_PACCEPT, // pending accept
};

union SockAddrAll
{
	struct sockaddr s;
	struct sockaddr_in v4;
	struct sockaddr_in6 v6;
};

class ServerImpl: public noncopyable
{
    public:

        ServerImpl();
        ~ServerImpl();

        // connect to addr, and add the corresponding socket to epoll for watching.
        SocketConnection* ConnectTo(const char* addr, int port, uintptr_t opaque);

        bool CloseSocket(int fd);
        // connect to ip:port, return the socket fd, not add to epoll for watching.
        int ListenTo(const char* addr, int port, uintptr_t opaque);

        // add socket denoted by fd to epoll for watching.
        bool WatchSocket(int fd, bool listen);
        bool WatchRawSocket(int fd, bool listen);
        bool UnwatchSocket(int fd);

        void SetWatchAcceptedSock(bool watch) { watchAccepted_ = watch; }

        int ReadBuffer(int fd, char* buffer, int sz);
        int SendBuffer(int fd, const char* buffer, int sz);

        int  GetConnNumber() const;
        void StartServer();
        void StopServer();
        void RunPoll(SocketEvent* res);

    private:

        inline void ResetSocketSlot(SocketConnection*) const;
        void ForceSocketClose(SocketConnection* so) const;
        SocketConnection* SetupSocketConnection(int fd, uintptr_t opaque, bool poll);

        int WaitPollerIfNecessary();

        SocketCode HandleAcceptReady(SocketConnection* sock, SocketConnection*& conn);
        SocketCode HandleConnectDone(SocketConnection* sock);

        void SetupServer();
        void ShutDownAllSockets();

    private:

        mutable int connNum_;

        int pollEventIndex_;
        int pollEventNum_;

        const int maxSocket_;

        bool isRunning_;

        bool watchAccepted_;

        SocketConnection* sockets_;
        PollEvent* pollEvent_;
        SocketPoll poller_;
};

static size_t CalcMaxFileDesc()
{
    struct rlimit rl;
    getrlimit(RLIMIT_NOFILE, &rl);

    slog(LOG_INFO, "calc max fd:%d", rl.rlim_max);
    return rl.rlim_max;
}

// SocketConnection definition.
SocketConnection::SocketConnection(ServerImpl* server)
    :server_(server)
{
}

SocketConnection::~SocketConnection()
{
}

int SocketConnection::SendBuffer(const char* buff, int sz)
{
    return server_->SendBuffer(fd_, buff, sz);
}

int SocketConnection::ReadBuffer(char* buff, int sz)
{
    return server_->ReadBuffer(fd_, buff, sz);
}

void SocketConnection::CloseConnection()
{
    server_->CloseSocket(fd_);
}

bool SocketConnection::IsConnected() const
{
    return status_ == SS_CONNECTED || status_ == SS_LISTENING;
}

uintptr_t SocketConnection::GetOpaqueValue() const
{
    return opaque_;
}

int SocketConnection::GetConnectionId() const
{
    return fd_;
}

// ServerImpl
ServerImpl::ServerImpl()
    :connNum_(0)
    ,pollEventIndex_(0)
    ,pollEventNum_(0)
    ,maxSocket_(CalcMaxFileDesc())
    ,isRunning_(false)
    ,watchAccepted_(false)
    ,sockets_(new SocketConnection[maxSocket_])
    ,pollEvent_(new PollEvent[maxSocket_])
    ,poller_()
{
    for (int i = 0; i < maxSocket_; ++i)
    {
        sockets_[i].status_ = SS_INVALID;
        sockets_[i].SetServerImpl(this);
    }
}

void ServerImpl::SetupServer()
{
    isRunning_ = true;
}

ServerImpl::~ServerImpl()
{
    ShutDownAllSockets();
    delete[] sockets_;
    delete[] pollEvent_;
}

int ServerImpl::GetConnNumber() const
{
    return connNum_;
}

void ServerImpl::ForceSocketClose(SocketConnection* sock) const
{
    assert(sock);

    if (sock->status_ == SS_INVALID) return;

    --connNum_;
    poller_.RemoveSocket(sock->fd_);
    ResetSocketSlot(sock);

    close(sock->fd_);
    slog(LOG_VERB, "force closing socket:%d", sock->fd_);
}

void ServerImpl::ShutDownAllSockets()
{
    for (int i = 0; i < maxSocket_; i++)
    {
        SocketConnection* so = &sockets_[i];
        ForceSocketClose(so);
    }

    isRunning_ = false;
}

SocketConnection* ServerImpl::SetupSocketConnection(int fd, uintptr_t opaque, bool poll)
{
    SocketConnection* so = &sockets_[fd];

    assert(so->status_ == SS_INVALID);

    so->fd_ = fd;
    so->opaque_ = opaque;

    if (poll && !poller_.AddSocket(fd, so))
    {
        slog(LOG_ERROR, "SetupSocketConnection failed, fd: %d, opaque:%d", fd, opaque);
        ResetSocketSlot(so);
        return NULL;
    }

    slog(LOG_VERB, "setup socket:%d", fd);
    return so;
}

void ServerImpl::ResetSocketSlot(SocketConnection* sock) const
{
    sock->status_ = SS_INVALID;
}

static int TryConnectTo(int fd, struct addrinfo* ai_ptr)
{
    int status = connect(fd, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
    if (status != 0 && errno != EINPROGRESS)
    {
        slog(LOG_ERROR, "connect failed, error:%s\n", strerror(errno));
        return -1;
    }

    if (status == 0) return 1;
    
    return 2;
}

// thread safe
static struct addrinfo* AllocSocketFd(SocketPredicateProc proc,
                                      const char* host, const char* port,
                                      int* sfd, int* stat)
{
    int status;
    int sock = -1;
    struct addrinfo ai_hints;
    struct addrinfo *ai_list = NULL;
    struct addrinfo *ai_ptr  = NULL;

    memset(&ai_hints, 0, sizeof(ai_hints));
    ai_hints.ai_family   = AF_UNSPEC;
    ai_hints.ai_socktype = SOCK_STREAM;
    ai_hints.ai_protocol = IPPROTO_TCP;

    status = getaddrinfo(host, port, &ai_hints, &ai_list);
    if (status) goto _failed;

    for (ai_ptr = ai_list; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next)
    {
#ifdef SOCK_NONBLOCK
        sock = socket(ai_ptr->ai_family, ai_ptr->ai_socktype | SOCK_NONBLOCK, ai_ptr->ai_protocol);
        if (sock < 0) continue;
#else
        sock = socket(ai_ptr->ai_family, ai_ptr->ai_socktype, ai_ptr->ai_protocol);
        if (sock < 0) continue;
        SocketPoll::SetSocketNonBlocking(sock);
#endif

        status = proc(sock, ai_ptr);
        if (status > 0) break;

        close(sock);
        sock = -1;
    }

    if (sock < 0) goto _failed;
    if (sfd) *sfd = sock;
    if (stat) *stat = status;

    return ai_ptr;

_failed:

    slog(LOG_ERROR, "alloc socket fd fail, target:host(%s),port(%s)\n", host, port);
    freeaddrinfo(ai_list);
    return NULL;
}

// in case of error occurs, let user handles the error.
// closing socket should be the last step to be taken
int ServerImpl::SendBuffer(int fd, const char* buffer, int sz)
{
    SocketConnection* sock = &sockets_[fd];

    if (sock->status_ == SS_INVALID || sock->fd_ != fd)
    {
        slog(LOG_ERROR, "send, invalid socketid,sock(%d)", fd);
        return -2;
    }

    assert(sock->status_ != SS_LISTENING);

    int n = write(fd, buffer, sz);
    if (n < 0)
    {
        if (EINTR == errno || EAGAIN == errno)
        {
            n = 0;
        }
        else
        {
            slog(LOG_ERROR, "server:write to %d(fd=%d) failed.", fd, sock->fd_);
            return -1;
        }
    }

    if (n < sz)
    {
        poller_.ModifySocket(sock->fd_, sock, true);
    }
    else
    {
        poller_.ModifySocket(sock->fd_, sock, false);
    }

    return n;
}

int ServerImpl::ReadBuffer(int fd, char* buffer, int sz)
{
    assert(fd >=0 && fd <= maxSocket_);
    SocketConnection* sock = &sockets_[fd];

    assert(sz);
    assert(sock->status_ != SS_INVALID);

    int n = (int)read(fd, buffer, sz);

    // epoll is set ot EPOLLONESHOT, need to rewatch the fd after reading.
    poller_.ModifySocket(sock->fd_, sock, false);

    if (n < 0)
    {
        if (errno == EINTR || EAGAIN == errno)
        {
            slog(LOG_DEBUG, "server: read sock done:EAGAIN\n");
        }
        else
        {
            slog(LOG_ERROR, "read sock error, fd(%d)", fd);
            return -1;
        }

        return 0;
    }
    else if (n == 0)
    {
        slog(LOG_VERB, "socket closed by remote, sock(%d)", fd);
        return -1;
    }

    return n;
}

SocketConnection* ServerImpl::ConnectTo(const char* host, int _port, uintptr_t opaque)
{
    char port[16];
    sprintf(port, "%d", _port);

    int sock;
    int status = 0;

    struct addrinfo* ai_ptr = NULL;
    ai_ptr = AllocSocketFd(&TryConnectTo, host, port, &sock, &status);
    if (sock < 0 || ai_ptr == NULL) return NULL;

    // alloc socket entity, and poll the socket
    SocketConnection* new_sock = SetupSocketConnection(sock, opaque, true);

    if (status == 1)
    {
        // convert addr into string
        struct sockaddr* addr = ai_ptr->ai_addr;
        void* sin_addr = (ai_ptr->ai_family == AF_INET)?
            (void*)&((struct sockaddr_in*)addr)->sin_addr:
            (void*)&((struct sockaddr_in6*)addr)->sin6_addr;

        inet_ntop(ai_ptr->ai_family, sin_addr, new_sock->buff_, sizeof(new_sock->buff_));

        new_sock->status_ = SS_CONNECTED;
    }
    else
    {
        new_sock->status_ = SS_CONNECTING;

        // socket is nonblocking, connection is not complete
        // need to set fd writable to track the status.
        poller_.ModifySocket(new_sock->fd_, new_sock, true);
    }

    return new_sock;
}

static int TryListenTo(int fd, struct addrinfo* ai_ptr)
{
    int reuse = 1;
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int));

    if (ret == -1) return -1;

    if (bind(fd, ai_ptr->ai_addr, ai_ptr->ai_addrlen) == -1) return -1;

    if (listen(fd, 64) == -1) return -1;

    return 1;
}

int ListenTo(const char* host, int _port)
{
    int status = 0;
    int listen_fd = -1;
    struct addrinfo* ai_ptr = NULL;

    char port[16];
    sprintf(port, "%d", _port);

    ai_ptr = AllocSocketFd(&TryListenTo, host, port, &listen_fd, &status);
    if (listen_fd < 0 || ai_ptr == NULL) return -1;

    return listen_fd;
}

int ServerImpl::ListenTo(const char* host, int _port, uintptr_t opaque)
{
    int listen_fd = ::ListenTo(host, _port);
    if (listen_fd < 0) return -1;

    // set up socket, but not put it into epoll yet.
    // call start socket to if user wants to.
    SocketConnection* new_sock = SetupSocketConnection(listen_fd, opaque, true);

    new_sock->status_ = SS_LISTENING;
    return listen_fd;
}

// make sure this function is thread safe
bool ServerImpl::CloseSocket(int fd)
{
    SocketConnection* sock = &sockets_[fd];
    if (sock->status_ == SS_INVALID || sock->fd_ != fd)
    {
        slog(LOG_WARN, "try to close bad socket, fd(%d)", fd);
        return false;
    }

    ForceSocketClose(sock);

    return true;
}

// make sure this function thread safe.
bool ServerImpl::WatchSocket(int fd, bool listen)
{
    SocketConnection* sock = &sockets_[fd];
    if (sock->status_ == SS_PACCEPT || sock->status_ == SS_INVALID)
    {
        if (!poller_.AddSocket(sock->fd_, sock))
        {
            ResetSocketSlot(sock);
            return false;
        }

        if (sock->status_ == SS_PACCEPT || (!listen && sock->status_ == SS_INVALID))
        {
            sock->status_ = SS_CONNECTED;
        }
        else
        {
            sock->status_ = SS_LISTENING;
        }

        return true;
    }

    return false;
}

bool ServerImpl::WatchRawSocket(int fd, bool listen)
{
    SocketConnection* sock = &sockets_[fd];
    if (sock->status_ != SS_INVALID) return false;

    sock->fd_ = fd;
    sock->status_ = listen? SS_LISTENING:SS_CONNECTED;

    if (!poller_.AddSocket(sock->fd_, sock))
    {
        ResetSocketSlot(sock);
        return false;
    }

    return true;
}

bool ServerImpl::UnwatchSocket(int fd)
{
    SocketConnection* conn = &sockets_[fd];

    if (conn->status_ == SS_INVALID) return false;
    if (!poller_.RemoveSocket(fd)) return false;

    ResetSocketSlot(conn);
    return true;
}

SocketCode ServerImpl::HandleConnectDone(SocketConnection* sock)
{
    int error;
    socklen_t len = sizeof(error);
    int code = getsockopt(sock->fd_, SOL_SOCKET, SO_ERROR, &error, &len);

    if (code < 0 || error) return SC_FAIL_CONN;

    sock->status_ = SS_CONNECTED;
    poller_.ModifySocket(sock->fd_, sock, false);

    // retrieve peer name of the connected socket.
    union SockAddrAll u;
    socklen_t slen = sizeof(u);

    if (getpeername(sock->fd_, &u.s, &slen) == 0)
    {
        void* sin_addr = (u.s.sa_family == AF_INET)?
            (void*)&u.v4.sin_addr:(void*)&u.v6.sin6_addr;

        inet_ntop(u.s.sa_family, sin_addr, sock->buff_, sizeof(sock->buff_));
    }

    return SC_CONNECTED;
}

SocketCode ServerImpl::HandleAcceptReady(SocketConnection* sock, SocketConnection*& conn)
{
    union SockAddrAll ua;
    socklen_t len = sizeof(ua);

#ifdef _GNU_SOURCE
    int client_fd = accept4(sock->fd_, &ua.s, &len, SOCK_NONBLOCK);
    if (client_fd < 0) return SC_ERROR;
#else
    int client_fd = accept(sock->fd_, &ua.s, &len);
    if (client_fd < 0) return SC_ERROR;

    SocketPoll::SetSocketNonBlocking(client_fd);
#endif

    ++connNum_;
    SocketConnection* new_sock = SetupSocketConnection(client_fd, sock->opaque_, watchAccepted_);

    conn = new_sock;
    if (watchAccepted_)
    {
        new_sock->status_ = SS_CONNECTED;
    }
    else
    {
        new_sock->status_ = SS_PACCEPT;
    }

    void* sin_addr = (ua.s.sa_family == AF_INET)?
        (void*)&ua.v4.sin_addr : (void*)&ua.v6.sin6_addr;

    inet_ntop(ua.s.sa_family, sin_addr, new_sock->buff_, sizeof(new_sock->buff_));

    return SC_SUCC;
}

int ServerImpl::WaitPollerIfNecessary()
{
    if (pollEventIndex_ != pollEventNum_) return 1;

    pollEventNum_ = poller_.WaitAll(pollEvent_, maxSocket_);

    pollEventIndex_ = 0;
    if (pollEventNum_ > 0) return 1;

    pollEventNum_ = 0;
    return -1;
}

/*
 * this function should be in a separated thread to poll the status of the sockets.
 */
void ServerImpl::RunPoll(SocketEvent* result)
{
    int ret = 0;
    static std::queue<SocketEvent> accept_queue;
    static std::queue<SocketEvent> read_write_queue;

    while (1)
    {
        if (!accept_queue.empty())
        {
            *result = accept_queue.front();
            accept_queue.pop();
            return;
        }

        if (!read_write_queue.empty())
        {
            *result = read_write_queue.front();
            read_write_queue.pop();
            return;
        }

        if ((ret = WaitPollerIfNecessary()) < 0) continue;

        while (pollEventIndex_ < pollEventNum_)
        {
            SocketEvent evt;
            PollEvent* event = &pollEvent_[pollEventIndex_++];
            SocketConnection* sock = (SocketConnection*)event->data;

            evt.conn = sock;

            switch (sock->status_)
            {
                case SS_CONNECTING:
                    {
                        evt.code = HandleConnectDone(sock);
                        read_write_queue.push(evt);
                    }
                    break;
                case SS_LISTENING:
                    {
                        int ret = HandleAcceptReady(sock, evt.conn);

                        if (ret == SC_SUCC)
                        {
                            evt.code = SC_ACCEPTED;
                            accept_queue.push(evt);
                        }
                        else
                        {
                            evt.code = SC_ERROR;
                            slog(LOG_WARN, "server accept erro");
                        }
                    }
                    break;
                case SS_INVALID:
                    {
                        slog(LOG_WARN, "server: invalid socket, fd(%d)", sock->fd_);
                    }
                    break;
                default:
                    {
                        if (event->write)
                        {
                            evt.code = SC_WRITE;
                        }

                        if (event->read)
                        {
                            evt.code = SC_READ;
                        }

                        read_write_queue.push(evt);
                    }
                    break;
            }
        }
    }
}

void ServerImpl::StartServer()
{
    if (isRunning_) return;

    SetupServer();
}

void ServerImpl::StopServer()
{
    if (!isRunning_) return;

    ShutDownAllSockets();
}

// SocketServer
SocketServer::SocketServer()
    :impl_(NULL)
{
    impl_ = new ServerImpl();
}

SocketServer::~SocketServer()
{
    delete impl_;
}

void SocketServer::StartServer()
{
    impl_->StartServer();
}

void SocketServer::StopServer()
{
    impl_->StopServer();
}

SocketConnection* SocketServer::ConnectTo(const char* ip, int port , uintptr_t opaque)
{
    return impl_->ConnectTo(ip, port, opaque);
}

int SocketServer::ListenTo(const char* ip, int port, uintptr_t opaque)
{
    return impl_->ListenTo(ip, port, opaque);
}

int SocketServer::SendBuffer(int fd, const char* data, int sz)
{
    return impl_->SendBuffer(fd, data, sz);
}

int SocketServer::ReadBuffer(int fd, char* data, int sz)
{
    return impl_->ReadBuffer(fd, data, sz);
}

bool SocketServer::CloseSocket(int fd)
{
    return impl_->CloseSocket(fd);
}

bool SocketServer::WatchSocket(int fd, bool listen)
{
    return impl_->WatchSocket(fd, listen);
}

bool SocketServer::WatchRawSocket(int fd, bool listen)
{
    return impl_->WatchRawSocket(fd, listen);
}

void SocketServer::SetWatchAcceptedSock(bool watch)
{
    impl_->SetWatchAcceptedSock(watch);
}

void SocketServer::RunPoll(SocketEvent* evt)
{
    impl_->RunPoll(evt);
}

bool SocketServer::UnwatchSocket(int fd)
{
    return impl_->UnwatchSocket(fd);
}

int SocketServer::GetConnNumber() const
{
    return impl_->GetConnNumber();
}

const int SocketServer::max_conn_id = CalcMaxFileDesc();

