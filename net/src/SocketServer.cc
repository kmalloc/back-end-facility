#include "SocketServer.h"
#include "SocketPoll.h"

#include "sys/Log.h"
#include "sys/AtomicOps.h"
#include "thread/Thread.h"

#include <sys/types.h>
#include <sys/socket.h>
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


#define SOCK_BITS (sizeof(short))
#define MAX_SOCKET (1 << SOCK_BITS)
#define MIN_READ_BUFFER (64)

typedef int (* SocketPredicateProc)(int, struct addrinfo*, void*);

enum SocketStatus
{
    SS_INVALID,
    SS_RESERVED,
    SS_LISTEN,
    SS_PLISTEN, // pending listen
    SS_CONNECTED,
    SS_CONNECTING,
    SS_PACCEPT, // pending accept
    SS_HALFCLOSE, // socket is sending pending buffer.
    SS_BIND
};

enum SocketCode
{
    SC_DATA,
    SC_CLOSE,
    SC_CONNECTED,
    SC_ACCEPT,
    SC_ERROR,
    SC_IERROR, // error ignored
    SC_EXIT,

    SC_SUCC
};

struct SocketBuffer
{
    int left_size;
    int raw_size;
    char* raw_buffer;
    char* current_ptr;
    SocketBuffer* next;
};

// wrapper of raw socket fd
struct SocketEntity
{
    int fd;
    int id;
    int type;
    uintptr_t opaque;
    int size; // fixed-size for each read operation.

    // buffer that is pending to send.
    // this list is only allowed to accessed from writer thread.
    SocketBuffer* head;
    SocketBuffer* tail;
};

// definitions of internal cmd.
struct RequestConnect
{
    int id;
    int port;
    uintptr_t opaque;

    char host[1];
};

struct RequestSend
{
    int id;
    int sz;
    char* buffer;
};

struct RequestClose
{
    int id;
    uintptr_t opaque;
};

struct RequestListen
{
    int id;
    int port;
    int backlog;
    uintptr_t opaque;

    char host[1];
};

struct RequestBind
{
    int id;
    int fd;
    uintptr_t opaque;
};

struct RequestEpoll
{
    int id;
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
        struct RequestSend send;
        struct RequestClose close;
        struct RequestListen listen;
        struct RequestEpoll epoll;
        struct RequestBind bind;
    } u;

    char padding[256];
};

union SockAddrAll {
	struct sockaddr s;
	struct sockaddr_in v4;
	struct sockaddr_in6 v6;
};

class ServerImpl: public ThreadBase
{
    public:

        ServerImpl();
        ~ServerImpl();

        int ReserveSocketSlot();

        // intermedia temp buffer.
        char m_buff[256];

        friend class SocketServer;

        // interface

        // connect to addr, and add the corresponding socket to epoll for watching.
        int ConnectTo(const char* addr, int port, uintptr_t opaque);

        // connect to ip:port, return the socket fd, not add to epoll for watching.
        int ListenTo(const char* addr, int port, uintptr_t opaque, int backlog);

        // add socket denoted by id to epoll for watching.
        void EnableEpollOnSocket(int id, uintptr_t opaque);

        // add socket fd to epoll for watching.
        int AddRawSocket(int fd, uintptr_t opaque);

        int SendData(int sock_id, const void* buffer, int sz);

        int BindConnection(int sock_fd, uintptr_t opaque);

        void CloseConnection(int sock_id, uintptr_t opaque);

        void StartServer();

        void StopServer();

    protected:

        SocketEntity* SetupSocket(int id, int fd, uintptr_t opaque, bool poll);
        void ResetSocketSlot(int id);
        void ForceSocketClose(SocketEntity* so, SocketMessage* res) const;

        int HandleReadReady(SocketEntity* sock, SocketMessage* result) const;
        int HandleAcceptReady(SocketEntity* sock, SocketMessage* result);
        int HandleConnectDone(SocketEntity* sock, SocketMessage* result);
        int SendPendingBuffer(SocketEntity* sock, SocketMessage* res) const;

        // function that is called in epoll_wait() handler.
        int ShutdownServer(void* buf, SocketMessage* result);
        int ConnectSocket(void* buf, SocketMessage* result);
        int CloseSocket(void* buf, SocketMessage* result);
        int BindRawSocket(void* buf, SocketMessage* result);
        int AddSocketToEpoll(void* buf, SocketMessage* result);
        int ListenSocket(void* buf, SocketMessage* result);
        int SendSocketBuffer(void* buffer, SocketMessage* res);

        int WaitEpollIfNecessary();
        int ProcessInternalCmd(SocketMessage* msg);

        // send cmd through pipe
        void SendInternalCmd(RequestPackage* req, char type, int len) const;

        // call epoll_wait, and handle the event accordingly 
        int Poll(SocketMessage* res);

        // separated thread to poll the file descriptor.
        // call epoll_wait.
        virtual void Run();

    private:

        // disable thread interface
        using ThreadBase::Start;

        enum
        {
            ACTION_BIND,
            ACTION_CONNECT,
            ACTION_LISTEN,
            ACTION_EPOLL,
            ACTION_CLOSE,
            ACTION_SEND,
            ACTION_STOP_SERVER,

            ACTION_NONE
        };

        void SetupServer();
        void ShutDownAllSockets();

        int m_pollEventIndex;
        int m_pollEventNum;

        int m_sendCtrlFd;
        int m_recvCtrlFd;

        // indexer for allocate id;
        int m_allocId;
        // TODO
        // set to the maximum number of file descriptor of process.
        // need to query ulimit -n.
        // for now, just set it to 0xffff
        const int m_maxSocket;

        SocketEntity m_sockets[MAX_SOCKET];
        PollEvent m_pollEvent[MAX_SOCKET];
        SocketPoll m_poll;

        typedef int (ServerImpl::* ActionHandler)(void*, SocketMessage*);
        static const ActionHandler m_actionHandler[];
};


const ServerImpl::ActionHandler ServerImpl::m_actionHandler[] =
{
   &ServerImpl::BindRawSocket,
   &ServerImpl::ConnectSocket,
   &ServerImpl::ListenSocket,
   &ServerImpl::AddSocketToEpoll,
   &ServerImpl::CloseSocket,
   &ServerImpl::SendSocketBuffer,
   &ServerImpl::ShutdownServer
};

static inline void ReleaseFrontBuffer(SocketEntity* sock)
{
    SocketBuffer* tmp = sock->head;
    sock->head = sock->head->next;
    free(tmp->raw_buffer);
    free(tmp);
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


ServerImpl::~ServerImpl()
{
}

int ServerImpl::ReserveSocketSlot()
{
    int i = 0;
    while (i < MAX_SOCKET)
    {
        int id = atomic_increment(&m_allocId);
        if (id < 0)
        {
            id = atomic_and_and_fetch(&m_allocId, ~(1 << (sizeof(int) - 1)));
        }

        SocketEntity* sock = &m_sockets[id%MAX_SOCKET];

        if (sock->type == SS_INVALID)
        {
            int ret = atomic_cas(&sock->type, SS_INVALID, SS_RESERVED);
            if (ret) return id;

            continue;
        }

        ++i;
    }

    return -1;
}

// ServerImpl
ServerImpl::ServerImpl()
    :m_pollEventIndex(0)
    ,m_pollEventNum(0)
    ,m_allocId(0)
    ,m_maxSocket(MAX_SOCKET)
    ,m_poll()
{
    SetupServer();
}

void ServerImpl::SetupServer()
{
    int fd[2];
    if (pipe(fd))
    {
        slog(LOG_ERROR, "server, create pipe fail");
        throw "pipe fail";
    }

    if (!m_poll.AddSocket(fd[0], NULL))
    {
        slog(LOG_ERROR, "server:add ctrl fd failed.");
        close(fd[0]);
        close(fd[1]);
        throw "add ctrl fd fail";
    }

    m_recvCtrlFd = fd[0];
    m_sendCtrlFd = fd[1];

    for (int i = 0; i < m_maxSocket; ++i)
    {
        m_sockets[i].head = NULL;
        m_sockets[i].tail = NULL;
        m_sockets[i].type = SS_INVALID;
    }
}

// release all pending send-buffer, and close socket.
void ServerImpl::ForceSocketClose(SocketEntity* sock, SocketMessage* result) const
{
    assert(sock);

    result->id = sock->id;
    result->data = 0;
    result->ud = 0;
    result->opaque = sock->opaque;

    if (sock->type == SS_INVALID) return;

    // release pending buffers.
    SocketBuffer* wb = sock->head;
    while (wb)
    {
        SocketBuffer* tmp = wb;
        wb = wb->next;
        free(tmp->raw_buffer);
        free(tmp);
    }

    sock->head = sock->tail = NULL;
    m_poll.RemoveSocket(sock->fd);
    close(sock->fd);

    sock->type = SS_INVALID;
}

void ServerImpl::ShutDownAllSockets()
{
    for (int i = 0; i < MAX_SOCKET; i++)
    {
        SocketMessage dummy;
        SocketEntity* so = &m_sockets[i];
        if (so->type != SS_RESERVED) ForceSocketClose(so, &dummy);
    }

    close(m_sendCtrlFd);
    close(m_recvCtrlFd);
}

SocketEntity* ServerImpl::SetupSocket(int id, int fd, uintptr_t opaque, bool poll)
{
    SocketEntity* so = &m_sockets[id % MAX_SOCKET];

    assert(so->type == SS_RESERVED);

    if (poll && !m_poll.AddSocket(fd, so))
    {
        so->type = SS_INVALID;
        slog(LOG_ERROR, "SetupSocket failed, id: %d, opaque:%d", id, opaque);
        return NULL;
    }

    so->id = id;
    so->fd = fd;
    so->size = MIN_READ_BUFFER;
    so->opaque = opaque;

    assert(so->head == NULL);
    assert(so->tail == NULL);

    return so;
}

void ServerImpl::ResetSocketSlot(int id)
{
    m_sockets[id%MAX_SOCKET].type = SS_INVALID;
}

static int TryConnectTo(int fd, struct addrinfo* ai_ptr, void*)
{
    int status = connect(fd, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
    if (status != 0 && errno != EINPROGRESS)
    {
        return -1;
    }

    return status;
}

// thread safe
static struct addrinfo* AllocSocketFd(SocketPredicateProc proc,
        const char* host, const char* port, int* sock_, int* stat, void* data)
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
    if (status != 0) goto _failed;

    for (ai_ptr = ai_list; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next)
    {
        sock = socket(ai_ptr->ai_family, ai_ptr->ai_socktype, ai_ptr->ai_protocol);
        if (sock < 0) continue;

        status = proc(sock, ai_ptr, data);
        if (status >= 0) break;

        close(sock);
        sock = -1;
    }

    if (sock < 0) goto _failed;

    if (stat) *stat = status;
    if (sock_) *sock_ = sock;

    return ai_ptr;

_failed:

    slog(LOG_ERROR, "alloc socket fd fail, target:host(%s),port(%s)\n", host, port);
    freeaddrinfo(ai_list);
    return NULL;
}

// this function should be called in writer thread(one) only.
int ServerImpl::SendPendingBuffer(SocketEntity* sock, SocketMessage* res) const
{
    SocketBuffer* buf = NULL;
    while ((buf = sock->head))
    {
        while (1)
        {
            int sz = write(sock->fd, buf->current_ptr, buf->left_size);
            if (sz < 0)
            {
                if (errno == EINTR) continue;
                else if (errno == EAGAIN) return SC_ERROR;

                ForceSocketClose(sock, res);
                return SC_CLOSE;
            }

            if (sz != buf->left_size)
            {
                buf->current_ptr += sz;
                buf->left_size -= sz;
                return SC_SUCC;
            }

            break;
        }

        ReleaseFrontBuffer(sock);
    }

    sock->tail = NULL;
    m_poll.ModifySocket(sock->fd, sock, false);

    if (sock->type == SS_HALFCLOSE) ForceSocketClose(sock, res);

    return SC_SUCC;
}

/* ----------------- internal actions ---------------------------------*/

// warn: should be called from writer thread only.
// buffer will be send immediately if no other buffers are pending to send.
// otherwise, current buffer will be linked to send buffer queue.
int ServerImpl::SendSocketBuffer(void* buffer, SocketMessage* res)
{
    RequestSend* req = (RequestSend*)buffer;

    int id  = req->id;
    SocketEntity* sock = &m_sockets[id % MAX_SOCKET];

    if (sock->type == SS_INVALID
            || sock->id != id
            || sock->type == SS_HALFCLOSE
            || sock->type == SS_PACCEPT)
    {
        free(req->buffer);
        return SC_SUCC;
    }

    assert(sock->type != SS_LISTEN && sock->type != SS_PLISTEN);

    if (sock->head)
    {
       SocketBuffer* buf = (SocketBuffer*)malloc(sizeof(SocketBuffer));
       buf->current_ptr = req->buffer;
       buf->left_size = req->sz;
       buf->raw_buffer = req->buffer;
       buf->raw_size   = req->sz;

       assert(sock->tail);
       assert(sock->tail->next == NULL);

       buf->next = NULL;
       sock->tail->next = buf;
       sock->tail = buf;

       return SC_SUCC;
    }

    int n = write(sock->fd, req->buffer, req->sz);
    if (n < 0)
    {
        if (EINTR == errno || EAGAIN == errno)
        {
            n = 0;
        }
        else
        {
            slog(LOG_ERROR, "server:write to %d(fd=%d) failed.", id, sock->fd);
            ForceSocketClose(sock, res);
            return SC_CLOSE;
        }
    }

    if (n == req->sz)
    {
        free(req->buffer);
        return SC_SUCC;
    }

    SocketBuffer* buf = (SocketBuffer*)malloc(sizeof(SocketBuffer));
    assert(buf);
    buf->next = NULL;
    buf->current_ptr = req->buffer + n;
    buf->left_size = req->sz - n;
    buf->raw_buffer = req->buffer;
    buf->raw_size = req->sz;

    sock->head = sock->tail = buf;
    m_poll.ModifySocket(sock->fd, sock, true);

    return SC_SUCC;
}

int ServerImpl::ConnectSocket(void* buffer, SocketMessage* res)
{
    RequestConnect* req = (RequestConnect*)buffer;

    int id = req->id;
    res->opaque = req->opaque;
    res->id     = req->id;
    res->ud     = 0;
    res->data   = NULL;

    char port[16];
    sprintf(port, "%d", req->port);

    int status = 0;
    int sock;

    struct addrinfo* ai_ptr = NULL;

    ai_ptr = AllocSocketFd(&TryConnectTo, req->host, port, &sock, &status, req);

    if (sock < 0 || ai_ptr == NULL) return SC_ERROR;

    // alloc socket entity, and poll the socket
    SocketEntity* new_sock = SetupSocket(id, sock, req->opaque, true);

    if (new_sock == NULL)
    {
        close(sock);
        goto _failed;
    }

    // epoll
    m_poll.SetSocketNonBlocking(sock);

    if (status == 0)
    {
        // convert addr into string
        struct sockaddr* addr = ai_ptr->ai_addr;
        void* sin_addr = (ai_ptr->ai_family == AF_INET)?
            (void*)&((struct sockaddr_in*)addr)->sin_addr:
            (void*)&((struct sockaddr_in6*)addr)->sin6_addr;

        inet_ntop(ai_ptr->ai_family, sin_addr, m_buff, sizeof(m_buff));

        new_sock->type = SS_CONNECTED;
        res->data = m_buff;

        return SC_CONNECTED;
    }
    else
    {
        new_sock->type = SS_CONNECTING;

        // since socket is nonblocking.
        // connecting is in the progress.
        // set writable to track status by epoll.
        m_poll.ModifySocket(new_sock->fd, new_sock, true);
    }

    return SC_SUCC;

_failed:

    ResetSocketSlot(id);
    return SC_ERROR;
}

static int TryListenTo(int fd, struct addrinfo* ai_ptr, void* data)
{
    int reuse = 1;
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int));

    if (ret == -1) return SC_SUCC;

    if (bind(fd, ai_ptr->ai_addr, ai_ptr->ai_addrlen) == -1) return SC_SUCC;

    if (listen(fd, ((RequestListen*)data)->backlog) == -1) return SC_SUCC;

    return 1;
}

int ServerImpl::ListenSocket(void* buffer, SocketMessage* res)
{
    RequestListen* req = (RequestListen*)buffer;

    int id = req->id;
    int listen_fd = -1;

    int sock;
    int status = 0;
    char port[16];

    struct addrinfo* ai_ptr = NULL;

    sprintf(port, "%d", req->port);
    ai_ptr = AllocSocketFd(&TryListenTo, req->host, port, &sock, &status, req);

    // set up socket, but not put it into epoll yet.
    // call start socket to if user wants to.
    SocketEntity* new_sock = SetupSocket(id, sock, req->opaque, false);

    if (new_sock == NULL) goto _failed;

    // socket is not in epoll, hence set type to pending listen.
    new_sock->type = SS_PLISTEN;

    return SC_SUCC;

_failed:

    close(sock);
    res->opaque = req->opaque;
    res->id = id;
    res->ud = 0;
    res->data = NULL;
    ResetSocketSlot(id);

    return SC_ERROR;
}

int ServerImpl::CloseSocket(void* buffer, SocketMessage* res)
{
    RequestClose* req = (RequestClose*)buffer;

    int id = req->id;

    SocketEntity* sock = &m_sockets[id%MAX_SOCKET];
    if (sock->type == SS_INVALID || sock->id != id)
    {
        res->id = id;
        res->opaque = req->opaque;
        res->ud = 0;
        res->data = NULL;
        return SC_CLOSE;
    }

    if (sock->head)
    {
        int type = SendPendingBuffer(sock, res);
        if (type != SC_SUCC) return type;
    }

    if (sock->head == NULL)
    {
        ForceSocketClose(sock, res);
        res->id = id;
        res->opaque = req->opaque;
        return SC_CLOSE;
    }

    // still have pending buffers.
    // socket will be closed after all buffers are send.
    sock->type = SS_HALFCLOSE;
    return SC_SUCC;
}

int ServerImpl::BindRawSocket(void* buffer, SocketMessage* res)
{
    RequestBind* req = (RequestBind*)buffer;

    int id = req->id;
    res->id = id;
    res->opaque = req->opaque;
    res->ud = 0;

    SocketEntity* sock = SetupSocket(id, req->fd, req->opaque, true);
    if (sock == NULL)
    {
        res->data = NULL;
        return SC_ERROR;
    }

    m_poll.SetSocketNonBlocking(req->fd);
    sock->type = SS_BIND;
    res->data = "binding";
    return SC_CONNECTED;
}

// add pending socket(acceptted, or open to listen) to epoll
int ServerImpl::AddSocketToEpoll(void* buffer, SocketMessage* res)
{
    RequestEpoll* req = (RequestEpoll*)buffer;

    int id = req->id;
    res->id = id;
    res->opaque = req->opaque;
    res->ud = 0;
    res->data = NULL;

    SocketEntity* sock = &m_sockets[id%MAX_SOCKET];
    if (sock->type == SS_PACCEPT || sock->type == SS_PLISTEN)
    {
        if (!m_poll.AddSocket(sock->fd, sock))
        {
            sock->type = SS_INVALID;
            return SC_ERROR;
        }

        sock->type = (sock->type == SS_PACCEPT)?SS_CONNECTED:SS_LISTEN;
        sock->opaque = req->opaque;
        res->data = "start";
        return SC_CONNECTED;
    }

    return SC_SUCC;
}

int ServerImpl::ShutdownServer(void*, SocketMessage* result)
{
    ShutDownAllSockets();

    result->opaque = 0;
    result->id = 0;
    result->ud = 0;
    result->data = NULL;

    return SC_EXIT;
}

int ServerImpl::ProcessInternalCmd(SocketMessage* result)
{
    char buffer[256];
    char header[2];

    ReadPipe(m_recvCtrlFd, header, sizeof(header));

    int type = header[0];
    int len  = header[1];

    if (type < 0 || type >= sizeof(m_actionHandler)/sizeof(m_actionHandler[0])) return SC_SUCC;

    ReadPipe(m_recvCtrlFd, buffer, len);

    return (this->*(m_actionHandler[type]))(buffer, result);
}

int ServerImpl::HandleReadReady(SocketEntity* sock, SocketMessage* result) const
{
    int sz = sock->size;
    char* buffer = (char*)malloc(sz);

    if (buffer == NULL) return SC_ERROR;

    int n = (int)read(sock->fd, buffer, sz);

    if (n < 0)
    {
        free(buffer);
        switch (errno)
        {
            case EINTR:
                break;
            case EAGAIN:
                slog(LOG_ERROR, "server: read sock done:EAGAIN\n");
                break;
            default:
                ForceSocketClose(sock, result);
                return SC_ERROR;
        }

        return SC_IERROR;
    }

    if (n == 0)
    {
        free(buffer);
        ForceSocketClose(sock, result);
        return SC_CLOSE;
    }

    if (sock->type == SS_HALFCLOSE) return SC_IERROR;

    if (n == sz) sock->size *= 2;
    else if (sz > MIN_READ_BUFFER && n * 2 < sz) sock->size /= 2;


    result->opaque = sock->opaque;
    result->id = sock->id;
    result->ud = n;
    result->data = buffer;

    return SC_DATA;
}

int ServerImpl::HandleConnectDone(SocketEntity* sock, SocketMessage* result)
{
    int error;
    socklen_t len = sizeof(error);
    int code = getsockopt(sock->fd, SOL_SOCKET, SO_ERROR, &error, &len);
    if (code < 0 || error)
    {
        ForceSocketClose(sock, result);
        return SC_ERROR;
    }
    else
    {
        sock->type = SS_CONNECTED;
        result->opaque = sock->opaque;
        result->id = sock->id;
        result->ud = 0;
        m_poll.ModifySocket(sock->fd, sock, false);

        // retrieve peer name of the connected socket.
        union SockAddrAll u;
        socklen_t slen = sizeof(u);
        if (getpeername(sock->fd, &u.s, &slen) == 0)
        {
            void* sin_addr = (u.s.sa_family == AF_INET)?
                (void*)&u.v4.sin_addr:(void*)&u.v6.sin6_addr;

            if (inet_ntop(u.s.sa_family, sin_addr, m_buff, sizeof(m_buff)))
            {
                result->data = m_buff;
                return SC_CONNECTED;
            }
        }

        result->data = NULL;
        return SC_CONNECTED;
    }
}

int ServerImpl::HandleAcceptReady(SocketEntity* sock, SocketMessage* result)
{
    union SockAddrAll ua;
    socklen_t len = sizeof(ua);
    int client_fd = accept(sock->fd, &ua.s, &len);
    if (client_fd < 0) return SC_ERROR;

    int id = ReserveSocketSlot();
    if (id < 0)
    {
        close(client_fd);
        return SC_ERROR;
    }

    m_poll.SetSocketNonBlocking(client_fd);
    SocketEntity* new_sock = SetupSocket(id, client_fd, sock->opaque, false);

    if (new_sock == NULL)
    {
        close(client_fd);
        return SC_ERROR;
    }

    new_sock->type = SS_PACCEPT;
    result->opaque = sock->opaque;
    result->id = sock->id;
    result->ud = id; // newly accepted socket id.
    result->data = NULL;

    void* sin_addr = (ua.s.sa_family == AF_INET)?
        (void*)&ua.v4.sin_addr : (void*)&ua.v6.sin6_addr;

    if (inet_ntop(ua.s.sa_family, sin_addr, m_buff, sizeof(m_buff)))
    {
        result->data = m_buff;
    }

    return SC_SUCC;
}

int ServerImpl::WaitEpollIfNecessary()
{
    if (m_pollEventIndex != m_pollEventNum) return 1;

    m_pollEventNum = m_poll.WaitAll(m_pollEvent, m_maxSocket);

    m_pollEventIndex = 0;

    if (m_pollEventNum > 0) return 1;

    m_pollEventNum = 0;
    return -1;
}

/*
 * this function should be in a separated thread to poll the status of the sockets.
 */
int ServerImpl::Poll(SocketMessage* result)
{
    while (1)
    {
        int ret = 0;
        if ((ret = WaitEpollIfNecessary()) < 0) return ret;

        PollEvent* event = &m_pollEvent[m_pollEventIndex++];
        SocketEntity* sock = (SocketEntity*)event->data;

        if (sock == NULL)
        {
            int type = ProcessInternalCmd(result);

            if (type == SC_SUCC) continue;

            return type;
        }

        switch (sock->type)
        {
            case SS_CONNECTING:
                return HandleConnectDone(sock, result);
            case SS_LISTEN:
                if (HandleAcceptReady(sock, result) == SC_SUCC)
                    return SC_ACCEPT;
                break;
            case SS_INVALID:
                slog(LOG_ERROR, "server: invalid socket\n");
                break;
            default:
                if (event->write)
                {
                    int type = SendPendingBuffer(sock, result);
                    if (type == SC_SUCC) break;

                    return type;
                }

                if (event->read)
                {
                    int type = HandleReadReady(sock, result);
                    if (type == SC_SUCC) break;

                    return type;
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
        int n = write(m_sendCtrlFd, &req->header[6], len + 2);
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

    int id = ReserveSocketSlot();
    req.u.connect.opaque = opaque;
    req.u.connect.id     = id;
    req.u.connect.port   = port;
    memcpy(req.u.connect.host, addr, len);
    req.u.connect.host[len] = '\0';

    SendInternalCmd(&req, ACTION_CONNECT, sizeof(req.u.connect) + len);
    return id;
}

// connect to add/port, return socket id, corresponding socket fd not added to epoll.
int ServerImpl::ListenTo(const char* addr, int port, uintptr_t opaque, int backlog)
{
    if (addr == NULL) return 0;

    RequestPackage req;
    int len = strlen(addr);

    if (len + sizeof(req.u.listen) > 256)
    {
        slog(LOG_ERROR, "server:invalid listen addr:%s\n", addr);
        return 0;
    }

    int id = ReserveSocketSlot();
    req.u.listen.opaque = opaque;
    req.u.listen.id = id;
    req.u.listen.port = port;
    req.u.listen.backlog = backlog;

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
    return id;
}

int ServerImpl::SendData(int sock_id, const void* buffer, int sz)
{
    SocketEntity* sock = &m_sockets[sock_id % m_maxSocket];
    if (sock->id != sock_id || sock->type == SS_INVALID) return -1;

    assert(sock->type != SS_RESERVED);

    RequestPackage req;
    req.u.send.id = sock_id;
    req.u.send.sz = sz;
    req.u.send.buffer = (char*)buffer;

    SendInternalCmd(&req, ACTION_SEND, sizeof(req.u.send));

    return 0;
}

int ServerImpl::AddRawSocket(int sock_fd, uintptr_t opaque)
{
    RequestPackage req;
    int id = ReserveSocketSlot();
    req.u.bind.opaque = opaque;
    req.u.bind.id = id;
    req.u.bind.fd = sock_fd;

    SendInternalCmd(&req, ACTION_BIND, sizeof(req.u.bind));

    return id;
}

void ServerImpl::EnableEpollOnSocket(int id, uintptr_t opaque)
{
    RequestPackage req;
    req.u.epoll.id = id;
    req.u.epoll.opaque = opaque;

    SendInternalCmd(&req, ACTION_EPOLL, sizeof(req.u.epoll));
}

void ServerImpl::CloseConnection(int sock_id, uintptr_t opaque)
{
    RequestPackage req;
    req.u.close.id = sock_id;
    req.u.close.opaque = opaque;

    SendInternalCmd(&req, ACTION_CLOSE, sizeof(req.u.close));
}

void ServerImpl::StartServer()
{
    Start();
}

void ServerImpl::StopServer()
{
    RequestPackage req;
    SendInternalCmd(&req, ACTION_STOP_SERVER, 0);
}

void ServerImpl::Run()
{
    int ret;

    while (1)
    {
        SocketMessage res;

        ret = Poll(&res);

        switch (ret)
        {
            case SC_EXIT:
                return;
            case SC_CLOSE:
                break;
            case SC_CONNECTED:
                break;
            case SC_DATA:
                break;
            case SC_ACCEPT:
                break;
            default:
                break;
        }
    }
}

// SocketServer
SocketServer::SocketServer()
    :m_impl(NULL)
{
    m_impl = new ServerImpl();
}

SocketServer::~SocketServer()
{
}

