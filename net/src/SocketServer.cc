#include "SocketServer.h"
#include "SocketPoll.h"

#include "sys/Log.h"
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

typedef int (* SocketPredicateProc)(int, struct addrinfo*, void*);

enum SocketStatus
{
    SS_INVALID,
    SS_LISTEN,
    SS_PLISTEN, // pending listen
    SS_PWATCH, // pending watch
    SS_CONNECTED,
    SS_CONNECTING,
    SS_PACCEPT, // pending accept
};

// wrapper of raw socket fd
struct SocketEntity
{
    int fd;
    uintptr_t opaque;
    SocketStatus type;
};

// definitions of internal cmd.
struct RequestConnect
{
    int fd;
    int port;
    uintptr_t opaque;

    char host[1];
};

struct RequestClose
{
    int fd;
    uintptr_t opaque;
};

struct RequestExit
{
    uintptr_t opaque;
};

struct RequestListen
{
    int fd;
    int port;
    int backlog;
    bool poll;
    uintptr_t opaque;

    char host[1];
};

struct RequestWatch
{
    int fd;
    uintptr_t opaque;
};

struct RequestPackage
{
    // first 6 bytes for alignment.
    // last 2 byts are: command, len
    char header[8];
    union
    {
        char buffer[256];
        struct RequestConnect connect;
        struct RequestClose close;
        struct RequestExit exit;
        struct RequestListen listen;
        struct RequestWatch epoll;
    } u;

    char padding[256];
};

union SockAddrAll
{
	struct sockaddr s;
	struct sockaddr_in v4;
	struct sockaddr_in6 v6;
};

class ServerImpl: public ThreadBase
{
    public:

        ServerImpl(SocketEventHandler handler);
        ~ServerImpl();

        // interface

        void RegisterSocketEventHandler(SocketEventHandler handler) { handler_ = handler; }

        // connect to addr, and add the corresponding socket to epoll for watching.
        int ConnectTo(const char* addr, int port, uintptr_t opaque);

        // connect to ip:port, return the socket fd, not add to epoll for watching.
        int ListenTo(const char* addr, int port, uintptr_t opaque, int backlog = 64, bool poll = true);

        // add socket denoted by fd to epoll for watching.
        bool WatchSocket(int fd);
        bool CloseSocket(int fd);

        void SetWatchAcceptedSock(bool watch) { watchAccepted_ = watch; }

        int ReadBuffer(int fd, void* buffer, int sz, bool watch);
        int SendBuffer(int fd, const void* buffer, int sz, bool watch);

        void StartServer();
        void StopServer(uintptr_t opaque);

    private:

        inline void ResetSocketSlot(SocketEntity*) const;
        void ForceSocketClose(SocketEntity* so, SocketMessage* res) const;
        SocketEntity* SetupSocketEntity(int fd, uintptr_t opaque, bool poll);

        int WaitPollerIfNecessary();
        SocketCode HandleReadReady(SocketEntity* sock, SocketMessage* result) const;
        SocketCode HandleWriteReady(SocketEntity* sock, SocketMessage* result) const;

        SocketCode HandleAcceptReady(SocketEntity* sock, SocketMessage* result);
        SocketCode HandleConnectDone(SocketEntity* sock, SocketMessage* result);
        SocketCode ProcessInternalCmd(SocketMessage* msg);

        // function that is called in epoll_wait() handler.
        SocketCode ShutdownServer(void* buf, SocketMessage* result);
        SocketCode ConnectSocket(void* buf, SocketMessage* result);
        SocketCode ListenSocket(void* buf, SocketMessage* result);

        // call epoll_wait, and handle the event accordingly
        SocketCode Poll(SocketMessage* res);

        void SetupServer();
        void ShutDownAllSockets();

        // send cmd through pipe
        void SendInternalCmd(RequestPackage* req, char type, int len) const;

        // separated thread to poll the file descriptor.
        // call epoll_wait.
        virtual void Run();

    private:

        using ThreadBase::Start;

        enum
        {
            ACTION_CONNECT,
            ACTION_LISTEN,
            ACTION_STOP_SERVER,

            ACTION_NONE
        };

        // intermedia temp buffer.
        char buff_[256];

        int pollEventIndex_;
        int pollEventNum_;

        int sendCtrlFd_;
        int recvCtrlFd_;

        // TODO
        // set to the maximum number of file descriptor of process.
        // need to query ulimit -n.
        // for now, just set it to 0xffff
        const int maxSocket_;

        bool isRunning_;

        bool watchAccepted_;

        SocketEntity* sockets_;
        PollEvent* pollEvent_;
        SocketPoll poll_;

        SocketEventHandler handler_;

        typedef SocketCode (ServerImpl::* ActionHandler)(void*, SocketMessage*);
        static const ActionHandler actionHandler_[];
};


const ServerImpl::ActionHandler ServerImpl::actionHandler_[] =
{
   &ServerImpl::ConnectSocket,
   &ServerImpl::ListenSocket,
   &ServerImpl::ShutdownServer
};

static size_t CalcMaxFileDesc()
{
    struct rlimit rl;
    getrlimit(RLIMIT_NOFILE, &rl);

    slog(LOG_INFO, "calc max fd:%d", rl.rlim_max);
    return rl.rlim_max;
}

static void ReadPipe(int fd, void* buff, int sz)
{
    while (1)
    {
        int n = read(fd, buff, sz);
        if (n < 0)
        {
            if (errno == EINTR) continue;

            slog(LOG_ERROR, "server: read pipe error:%s.\n", strerror(errno));
            return;
        }

        assert(n == sz);
        return;
    }
}

// ServerImpl
ServerImpl::ServerImpl(SocketEventHandler handler)
    :pollEventIndex_(0)
    ,pollEventNum_(0)
    ,maxSocket_(CalcMaxFileDesc())
    ,isRunning_(false)
    ,watchAccepted_(false)
    ,sockets_(new SocketEntity[maxSocket_])
    ,pollEvent_(new PollEvent[maxSocket_])
    ,poll_()
    ,handler_(handler)
{
    for (int i = 0; i < maxSocket_; ++i)
    {
        sockets_[i].type = SS_INVALID;
    }
}

ServerImpl::~ServerImpl()
{
    ShutDownAllSockets();
    delete[] sockets_;
    delete[] pollEvent_;
}

void ServerImpl::SetupServer()
{
    int fd[2];
    if (pipe(fd))
    {
        slog(LOG_ERROR, "server, create pipe fail");
        throw "pipe fail";
    }

    if (!poll_.AddSocket(fd[0], NULL))
    {
        slog(LOG_ERROR, "server:add ctrl fd failed.");
        close(fd[0]);
        close(fd[1]);
        throw "add ctrl fd fail";
    }

    recvCtrlFd_ = fd[0];
    sendCtrlFd_ = fd[1];

    for (int i = 0; i < maxSocket_; ++i)
    {
        sockets_[i].type = SS_INVALID;
    }

    isRunning_ = true;

    slog(LOG_INFO, "Server Setup, slot number:%d\n", maxSocket_);
}

// release all pending send-buffer, and close socket.
// make sure this function is thread safe.
void ServerImpl::ForceSocketClose(SocketEntity* sock, SocketMessage* result) const
{
    assert(sock);

    result->fd = sock->fd;
    result->data = 0;
    result->ud = 0;
    result->opaque = sock->opaque;

    if (sock->type == SS_INVALID) return;

    ResetSocketSlot(sock);
    poll_.RemoveSocket(sock->fd);
    close(sock->fd);

    slog(LOG_VERB, "force closing socket:%d", sock->fd);
}

void ServerImpl::ShutDownAllSockets()
{
    for (int i = 0; i < maxSocket_; i++)
    {
        SocketMessage dummy;
        SocketEntity* so = &sockets_[i];
        ForceSocketClose(so, &dummy);
    }

    close(sendCtrlFd_);
    close(recvCtrlFd_);

    isRunning_ = false;
}

SocketEntity* ServerImpl::SetupSocketEntity(int fd, uintptr_t opaque, bool poll)
{
    SocketEntity* so = &sockets_[fd];

    assert(so->type == SS_INVALID);

    so->fd = fd;
    so->opaque = opaque;

    if (poll && !poll_.AddSocket(fd, so))
    {
        slog(LOG_ERROR, "SetupSocketEntity failed, fd: %d, opaque:%d", fd, opaque);
        ResetSocketSlot(so);
        return NULL;
    }

    slog(LOG_VERB, "setup socket:%d", fd);
    return so;
}

void ServerImpl::ResetSocketSlot(SocketEntity* sock) const
{
    sock->type = SS_INVALID;
}

static int TryConnectTo(int fd, struct addrinfo* ai_ptr, void*)
{
    int status = connect(fd, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
    if (status != 0 && errno != EINPROGRESS)
    {
        slog(LOG_ERROR, "connect failed, error:%s\n", strerror(errno));
        return -1;
    }

    return 1;
}

// thread safe
static struct addrinfo* AllocSocketFd(SocketPredicateProc proc,
        const char* host, const char* port, int* _sock_, int* stat, void* data)
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

        status = proc(sock, ai_ptr, data);
        if (status >= 0) break;

        close(sock);
        sock = -1;
    }

    if (sock < 0) goto _failed;

    if (stat) *stat = status;
    if (_sock_) *_sock_ = sock;

    return ai_ptr;

_failed:

    slog(LOG_ERROR, "alloc socket fd fail, target:host(%s),port(%s)\n", host, port);
    freeaddrinfo(ai_list);
    return NULL;
}

int ServerImpl::SendBuffer(int fd, const void* buffer, int sz, bool watch)
{
    SocketEntity* sock = &sockets_[fd];

    if (sock->type == SS_INVALID || sock->fd != fd)
    {
        return -2;
    }

    assert(sock->type != SS_LISTEN && sock->type != SS_PLISTEN);

    int n = write(fd, buffer, sz);
    if (n < 0)
    {
        if (EINTR == errno || EAGAIN == errno)
        {
            n = 0;
        }
        else
        {
            slog(LOG_ERROR, "server:write to %d(fd=%d) failed.", fd, sock->fd);

            SocketMessage dummy;
            ForceSocketClose(sock, &dummy);
            return -1;
        }
    }

    if (n < sz && watch)
    {
        if(!poll_.AddSocket(fd, sock, true))
        {
            slog(LOG_ERROR, "poll socket failed, error:%s", strerror(errno));
            SocketMessage dummy;
            ForceSocketClose(sock, &dummy);
            return -3;
        }
    }

    return n;
}

int ServerImpl::ReadBuffer(int fd, void* buffer, int sz, bool watch)
{
    SocketEntity* sock = &sockets_[fd];

    assert(sz);
    assert(sock->type != SS_INVALID);

    int n = (int)read(fd, buffer, sz);
    if (n < 0)
    {
        if (errno == EINTR || EAGAIN == errno)
        {
            slog(LOG_DEBUG, "server: read sock done:EAGAIN\n");
        }
        else
        {
            slog(LOG_ERROR, "read sock(%d) error, closing", sock->fd);
            SocketMessage dummy;
            ForceSocketClose(sock, &dummy);
            return -1;
        }
    }
    else if (n == 0)
    {
        slog(LOG_VERB, "socket closed by remote, sock(%d)", fd);
        SocketMessage dummy;
        ForceSocketClose(sock, &dummy);
        return 0;
    }

    if (watch)
    {
        if(!poll_.AddSocket(fd, sock))
        {
            slog(LOG_ERROR, "poll socket failed, error:%s", strerror(errno));
            SocketMessage dummy;
            ForceSocketClose(sock, &dummy);
            return -2;
        }
    }

    return n;
}

/* ----------------- internal actions ---------------------------------*/

SocketCode ServerImpl::ConnectSocket(void* buffer, SocketMessage* res)
{
    RequestConnect* req = (RequestConnect*)buffer;

    res->opaque = req->opaque;
    res->ud   = 0;
    res->data   = NULL;

    char port[16];
    sprintf(port, "%d", req->port);

    int status = 0;
    int sock;

    struct addrinfo* ai_ptr = NULL;

    ai_ptr = AllocSocketFd(&TryConnectTo, req->host, port, &sock, &status, req);

    if (sock < 0 || ai_ptr == NULL) return SC_ERROR;

    res->fd = sock;

    // alloc socket entity, and poll the socket
    SocketEntity* new_sock = SetupSocketEntity(sock, req->opaque, true);

    if (status == 0)
    {
        // convert addr into string
        struct sockaddr* addr = ai_ptr->ai_addr;
        void* sin_addr = (ai_ptr->ai_family == AF_INET)?
            (void*)&((struct sockaddr_in*)addr)->sin_addr:
            (void*)&((struct sockaddr_in6*)addr)->sin6_addr;

        inet_ntop(ai_ptr->ai_family, sin_addr, buff_, sizeof(buff_));

        new_sock->type = SS_CONNECTED;
        res->data = buff_;

        return SC_CONNECTED;
    }
    else
    {
        new_sock->type = SS_CONNECTING;

        // since socket is nonblocking.
        // connecting is in the progress.
        // set writable to track status by epoll.
        poll_.ModifySocket(new_sock->fd, new_sock, true);
    }

    return SC_SUCC;
}

static int TryListenTo(int fd, struct addrinfo* ai_ptr, void* data)
{
    int reuse = 1;
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int));

    if (ret == -1) return -1;

    if (bind(fd, ai_ptr->ai_addr, ai_ptr->ai_addrlen) == -1) return -1;

    if (listen(fd, ((RequestListen*)data)->backlog) == -1) return -1;

    return 1;
}

SocketCode ServerImpl::ListenSocket(void* buffer, SocketMessage* res)
{
    RequestListen* req = (RequestListen*)buffer;

    int listen_fd = -1;

    int status = 0;
    char port[16];

    struct addrinfo* ai_ptr = NULL;

    res->opaque = req->opaque;

    sprintf(port, "%d", req->port);
    ai_ptr = AllocSocketFd(&TryListenTo, req->host, port, &listen_fd, &status, req);

    if (listen_fd < 0 || ai_ptr == NULL) return SC_ERROR;

    res->fd = listen_fd;

    // set up socket, but not put it into epoll yet.
    // call start socket to if user wants to.
    SocketEntity* new_sock = SetupSocketEntity(listen_fd, req->opaque, req->poll);

    SocketCode ret;
    if (req->poll)
    {
        new_sock->type = SS_LISTEN;
        ret = SC_LISTENED;
    }
    else
    {
        new_sock->type = SS_PLISTEN;
        ret = SC_SUCC;
    }

    res->ud = 0;
    res->data = NULL;

    return ret;
}

// make sure this function is thread safe
bool ServerImpl::CloseSocket(int fd)
{
    SocketEntity* sock = &sockets_[fd];

    if (sock->type == SS_INVALID || sock->fd != fd)
    {
        slog(LOG_WARN, "try to close bad socket, fd(%d)", fd);
        return false;
    }

    SocketMessage res;
    ForceSocketClose(sock, &res);
}

// make sure this function thread safe.
bool ServerImpl::WatchSocket(int fd)
{
    SocketEntity* sock = &sockets_[fd];
    if (sock->type == SS_PACCEPT || sock->type == SS_PLISTEN || sock->type == SS_PWATCH)
    {
        if (!poll_.AddSocket(sock->fd, sock))
        {
            ResetSocketSlot(sock);
            return false;
        }

        if (sock->type == SS_PWATCH || sock->type == SS_PACCEPT)
        {
            sock->type = SS_CONNECTED;
        }
        else
        {
            sock->type = SS_LISTEN;
        }

        return true;
    }

    return false;
}

SocketCode ServerImpl::ShutdownServer(void* buffer, SocketMessage* result)
{
    RequestExit* req = (RequestExit*) buffer;

    ShutDownAllSockets();

    result->opaque = req->opaque;
    result->fd = 0;
    result->ud = 0;
    result->data = NULL;

    return SC_EXIT;
}

SocketCode ServerImpl::ProcessInternalCmd(SocketMessage* result)
{
    char buffer[256];
    char header[2];

    ReadPipe(recvCtrlFd_, header, sizeof(header));

    int type = header[0];
    int len  = header[1];

    if (type < 0 || type >= sizeof(actionHandler_)/sizeof(actionHandler_[0])) return SC_SUCC;

    ReadPipe(recvCtrlFd_, buffer, len);

    return (this->*(actionHandler_[type]))(buffer, result);
}

SocketCode ServerImpl::HandleReadReady(SocketEntity* sock, SocketMessage* result) const
{
    result->fd = sock->fd;
    result->opaque = sock->opaque;

    // socket may already closed before read event is handled
    if (sock->type == SS_INVALID) return SC_IERROR;

    // remove from poller.
    poll_.RemoveSocket(sock->fd);
    return SC_READREADY;
}

SocketCode ServerImpl::HandleWriteReady(SocketEntity* sock, SocketMessage* result) const
{
    result->fd = sock->fd;
    result->opaque = sock->opaque;

    // socket may already closed before read event is handled
    if (sock->type == SS_INVALID) return SC_IERROR;

    // remove from poller.
    poll_.RemoveSocket(sock->fd);
    return SC_WRITEREADY;
}

SocketCode ServerImpl::HandleConnectDone(SocketEntity* sock, SocketMessage* result)
{
    int error;
    socklen_t len = sizeof(error);
    int code = getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, &error, &len);

    result->fd = sock->fd;
    result->opaque = sock->opaque;

    if (code < 0 || error)
    {
        ForceSocketClose(sock, result);
        return SC_CLOSE;
    }
    else
    {
        sock->type = SS_CONNECTED;
        result->ud = 0;
        poll_.ModifySocket(sock->fd, sock, false);

        // retrieve peer name of the connected socket.
        union SockAddrAll u;
        socklen_t slen = sizeof(u);
        if (getpeername(sock->fd, &u.s, &slen) == 0)
        {
            void* sin_addr = (u.s.sa_family == AF_INET)?
                (void*)&u.v4.sin_addr:(void*)&u.v6.sin6_addr;

            if (inet_ntop(u.s.sa_family, sin_addr, buff_, sizeof(buff_)))
            {
                result->data = buff_;
                return SC_CONNECTED;
            }
        }

        result->data = NULL;
        return SC_CONNECTED;
    }
}

SocketCode ServerImpl::HandleAcceptReady(SocketEntity* sock, SocketMessage* result)
{
    union SockAddrAll ua;
    socklen_t len = sizeof(ua);

    result->fd = sock->fd;
    result->opaque = sock->opaque;

#ifdef _GNU_SOURCE
    int client_fd = accept4(sock->fd, &ua.s, &len, SOCK_NONBLOCK);
    if (client_fd < 0) return SC_ERROR;
#else
    int client_fd = accept(sock->fd, &ua.s, &len);
    if (client_fd < 0) return SC_ERROR;

    SocketPoll::SetSocketNonBlocking(client_fd);
#endif

    SocketEntity* new_sock = SetupSocketEntity(client_fd, sock->opaque, watchAccepted_);

    if (watchAccepted_) new_sock->type = SS_CONNECTED;
    else new_sock->type = SS_PACCEPT;

    result->opaque = sock->opaque;
    result->ud = client_fd; // newly accepted socket fd.
    result->data = NULL;

    void* sin_addr = (ua.s.sa_family == AF_INET)?
        (void*)&ua.v4.sin_addr : (void*)&ua.v6.sin6_addr;

    if (inet_ntop(ua.s.sa_family, sin_addr, buff_, sizeof(buff_)))
    {
        result->data = buff_;
    }

    return SC_SUCC;
}

int ServerImpl::WaitPollerIfNecessary()
{
    if (pollEventIndex_ != pollEventNum_) return 1;

    pollEventNum_ = poll_.WaitAll(pollEvent_, maxSocket_);

    pollEventIndex_ = 0;

    if (pollEventNum_ > 0) return 1;

    pollEventNum_ = 0;
    return -1;
}

/*
 * this function should be in a separated thread to poll the status of the sockets.
 */
SocketCode ServerImpl::Poll(SocketMessage* result)
{
    while (1)
    {
        int ret = 0;
        if ((ret = WaitPollerIfNecessary()) < 0) continue;

        PollEvent* event = &pollEvent_[pollEventIndex_++];
        SocketEntity* sock = (SocketEntity*)event->data;

        if (sock == NULL)
        {
            SocketCode type = ProcessInternalCmd(result);

            if (type == SC_SUCC) continue;

            return type;
        }

        switch (sock->type)
        {
            case SS_CONNECTING:
                return HandleConnectDone(sock, result);
            case SS_LISTEN:
                {
                    int ret = HandleAcceptReady(sock, result);
                    if (ret == SC_SUCC)
                    {
                        return SC_ACCEPTED;
                    }
                    else
                    {
                        slog(LOG_WARN, "server accept erro");
                    }
                }
                break;
            case SS_INVALID:
                slog(LOG_WARN, "server: invalid socket, fd(%d)", sock->fd);
                break;
            default:
                if (event->write)
                {
                    return HandleWriteReady(sock, result);
                }

                if (event->read)
                {
                    return HandleReadReady(sock, result);
                }
                break;
        }
    }
}

void ServerImpl::SendInternalCmd(RequestPackage* req, char type, int len) const
{
    req->header[6] = (uint8_t)type;
    req->header[7] = (uint8_t)len;

    while (1)
    {
        int n = write(sendCtrlFd_, &req->header[6], len + 2);
        if (n < 0)
        {
            if (errno != EINTR)
            {
                slog(LOG_ERROR, "server:send cmd failed:%s.\n", strerror(errno));
            }

            continue;
        }

        assert(n == len + 2);
        return;
    }
}

// connect to addr:port, and add socket to epoll for watching.
int ServerImpl::ConnectTo(const char* addr, int port, uintptr_t opaque)
{
    if (addr == NULL) return 0;

    RequestPackage req;
    int len = strlen(addr);

    if (len + sizeof(req.u.connect) > 256)
    {
        slog(LOG_ERROR, "server: connect err, invalid addr:%s\n", addr);
        return 0;
    }

    req.u.connect.opaque = opaque;
    req.u.connect.port   = port;
    memcpy(req.u.connect.host, addr, len);
    req.u.connect.host[len] = '\0';

    SendInternalCmd(&req, ACTION_CONNECT, sizeof(req.u.connect) + len);
    return 0;
}

// connect to add/port, return socket fd, corresponding socket fd not added to epoll.
int ServerImpl::ListenTo(const char* addr, int port, uintptr_t opaque, int backlog, bool poll)
{
    if (addr == NULL) return 0;

    RequestPackage req;
    int len = strlen(addr);

    if (len + sizeof(req.u.listen) > 256)
    {
        slog(LOG_ERROR, "server:invalid listen addr:%s\n", addr);
        return 0;
    }

    req.u.listen.opaque = opaque;
    req.u.listen.port = port;
    req.u.listen.backlog = backlog;
    req.u.listen.poll = poll;

    if (len == 0)
    {
        req.u.listen.host[0] = '\0';
    }
    else
    {
        int len = strlen(addr);
        memcpy(req.u.listen.host, addr, len);
        req.u.listen.host[len] = '\0';
    }

    SendInternalCmd(&req, ACTION_LISTEN, sizeof(req.u.listen) + len);
    return 0;
}

void ServerImpl::StartServer()
{
    if (isRunning_) return;

    SetupServer();
    ThreadBase::Start();
}

void ServerImpl::StopServer(uintptr_t opaque)
{
    if (!isRunning_) return;

    RequestPackage req;
    req.u.exit.opaque = opaque;
    SendInternalCmd(&req, ACTION_STOP_SERVER, sizeof(req.u.exit));
}

void ServerImpl::Run()
{
    while (1)
    {
        SocketEvent event;

        event.code = Poll(&event.msg);

        handler_(event);
    }
}

// SocketServer
SocketServer::SocketServer()
    :impl_(NULL)
{
    impl_ = new ServerImpl(&SocketServer::DefaultSockEventHandler);
}

SocketServer::~SocketServer()
{
    delete impl_;
}

void SocketServer::RegisterSocketEventHandler(SocketEventHandler handler)
{
    impl_->RegisterSocketEventHandler(handler);
}

int SocketServer::StartServer(const char* ip, int port, uintptr_t opaque)
{
    impl_->StartServer();

    return impl_->ListenTo(ip, port, opaque);
}

void SocketServer::StopServer(uintptr_t opaque)
{
    impl_->StopServer(opaque);
}

int SocketServer::ConnectTo(const char* ip, int port , uintptr_t opaque)
{
    return impl_->ConnectTo(ip, port, opaque);
}

int SocketServer::ListenTo(const char* ip, int port, uintptr_t opaque)
{
    return impl_->ListenTo(ip, port, opaque);
}

int SocketServer::SendBuffer(int fd, const void* data, int sz, bool watch)
{
    return impl_->SendBuffer(fd, data, sz, watch);
}

int SocketServer::ReadBuffer(int fd, void* data, int sz, bool watch)
{
    return impl_->ReadBuffer(fd, data, sz, watch);
}

bool SocketServer::CloseSocket(int fd)
{
    return impl_->CloseSocket(fd);
}

bool SocketServer::WatchSocket(int fd)
{
    return impl_->WatchSocket(fd);
}

void SocketServer::DefaultSockEventHandler(SocketEvent event)
{
}

void SocketServer::SetWatchAcceptedSock(bool watch)
{
    impl_->SetWatchAcceptedSock(watch);
}

const int SocketServer::max_conn_id = CalcMaxFileDesc();

