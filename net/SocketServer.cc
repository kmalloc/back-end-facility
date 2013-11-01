#include "SocketServer.h"

#include "sys/Log.h"
#include "sys/AtomicOps.h"
#include "misc/SocketPoll.h"
#include "thread/Thread.h"

#define SOCK_BITS (sizeof(short))
#define MAX_SOCKET (1 << SOCK_BITS)
#define MIN_READ_BUFFER (64)

typedef int (* OnSocketAllocCb)(int, struct addrinfo*, void*);

enum SocketStatus
{
    Socket_Invalid,
    Socket_Reserved,
    Socket_Listen,
    socket_PListen,
    Socket_PendingListen,
    Socket_Connected,
    Socket_Connecting,
    Socket_PAccepted,
    Socket_HalfClose,
    Socket_Bind
};

struct SocketBuffer
{
    void* raw_buffer;
    void* current_ptr;
    int left_size;
    int raw_size;
};

struct SocketEntity
{
    int fd;
    int socketid;
    int type;
    uintptr_t opaque;
    int size; // fixed-size for each read operation.
    
    // buffer that is pending to send.
    // this list is only allowed to accessed from writer thread.
    SocketBuffer* head;
    SocketBuffer* tail;

    SocketBuffer* GetFrontBuff() const { return head; }
    void ReleaseFrontBuffer() 
    { 
        SocketBuffer* tmp = head;
        head = head->next;
        free(tmp->raw_buffer);
        free(tmp);
    }
};

struct request_connect
{
    int id;
    int port;
    uintptr_t opaque;

    char host[1];
};

struct request_send
{
    int id;
    int sz;
    uintptr_t opaque;
};

struct request_close
{
    int id;
    uintptr_t opaque;
};

struct request_listen
{
    int id;
    int port;
    int backlog;
    uintptr_t opaque;

    char host[1];
};

struct request_bind
{
    int id;
    int fd;
    uintptr_t opaque;
};

struct request_start
{
    int id;
    uintptr_t opaque;
};

struct CmdPackage
{
    // first 6 bytes for alignment.
    // last 2 byts are: command, len
    char header[8];
    union
    {
        char buffer[256];
        struct request_connect connect;
        struct request_send send;
        struct request_close close;
        struct request_listen listen;
        struct request_start start;
    } ud;

    char padding[256];
};

class SocketServerImpl: public ThreadBase
{
    public:

        SocketServerImpl();
        ~SocketServerImpl();

        int ReserveSocketSlot();

        // intermedia temp buffer.
        char m_buff[];

        friend class SocketServer;

    protected:

        int  Connect(const string& ip, int port);
        int  StartServer();
        int  StartListen(const string& ip, int port);
        int  SendBuffer(int id, void* data, int len);

        void CloseSocket(SocketEntity* sock, SocketMessage* result);
        void ForceCloseSocket(SocketEntity* so, SocketMessage* res);
        void SetupSocket(int id, int fd, uintptr_t opaque, bool poll); 
        inline void ResetSocketSlot(int id);

        int  AllocSocketFd(const char* host, const char* port,
                void* buff, int len, int* status);

        // 
        int  QueueSocketBuffer(request_send* req, socket_message* res);

        virtual void Run();

    private:

        void SetupServer();
        void ShutDown();

        int Poll(SocketMessage& msg);

        // disable thread interface
        using ThreadBase::Start();

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

        SocketEntity m_sockets[];
        SocketPoll m_poll;
};


static void ReadPipe(int fd, void* buff, int sz)
{
    while (1)
    {
        int n = read(fd, buffer, sz);
        if (n < 0)
        {
            if (errno == EINTR) continue;

            slog(LOG_ERROR, "socket-server: read pipe error:%s.\n", strerror(errno));
            return;
        }

        assert(n == sz);
        return;
    }
}

SocketServerImpl::~SocketServerImpl()
{
}

int SocketServerImpl::ReserveSocketSlot()
{
    int i = 0;
    while (i < MAX_SOCKET)
    {
        int id = atomic_increment(&m_allocId);
        if (id < 0)
        {
            id = atomic_and_and_fetch(&m_allocId, ~(1 << (sizeof(int) - 1)));
        }

        struct SocketEntity* sock = &m_sockets[id%MAX_SOCKET]; 

        if (sock->type == Socket_Invalid)
        {
            int ret = atomic_cas(&sock->type, Socket_Invalid, Socket_Reserved); 
            if (ret) return id;

            continue;
        }

        ++i;
    }

    return -1;
}

// SocketServerImpl
//
SocketServerImpl::SocketServerImpl()
    :m_buff[128]
    ,m_pollEventIndex(0)
    ,m_pollEventNum(0)
    ,m_allocId(0)
    ,m_maxSocket(MAX_SOCKET)
    ,m_sockets[m_maxSocket]
    ,m_poll()
{
    SetupServer();
}

void SocketServerImpl::SetupServer()
{
    int fd[2];
    if (pipe(fd))
    {
        slog(LOG_ERROR, "socketserver, create pipe fail");
        throw "pipe fail";
    }

    if (!m_poll.AddSocket(fd[0], NULL))
    {
        slog(LOG_ERROR, "socketserver:add ctrl fd failed.");
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
        m_sockets[i].type = Socket_Invalid;
    }
}

// release all pending send-buffer, and close socket.
void SocketServerImpl::ForceCloseSocket(SocketEntity* sock, SocketMessage* result)
{
    assert(sock);

    result->socketid = socket->id;
    result->data = 0;
    result->size = 0;
    result->opaque = sock->opaque;
    if (sock->type == Socket_Invalid) return;

    assert(sock->type != Socket_Invalid);

    SocketBuffer* wb = sock->head;
    while (wb)
    {
        SocketBuffer* tmp = wb;
        wb = wb->next;
        free(tmp->raw_buffer);
        free(tmp);
    }

    sock->head = sock->tail = NULL;
    m_poll.DeleteSocket(sock->fd);
    close(sock->fd);

    sock->type = Socket_Invalid;
}

void SocketServerImpl::ShutDown()
{
    socket_message dummy;
    for (int i = 0; i < MAX_SOCKET; i++)
    {
        SocketEntity* so = &m_sockets[i];
        if (so->type != SOCKET_TYPE_RESERVE) ForceCloseSocket(so, &dummy);
    }

    close(m_sendCtrlFd);
    close(m_recvCtrlFd);
}

SocketEntity* SocketServerImpl::SetupSocket(int id, int fd,
        uintptr_t opaque, bool poll)
{
    SocketEntity* so = &m_sockets[id % MAX_SOCKET];
    assert(so->type == Socket_Reserved);

    if (poll)
    {
        if (!m_poll.AddSocket(fd, so))
        {
            so->type = Socket_Invalid;
            slog(LOG_ERROR, "SetupSocket failed, id: %d, opaque:%d", id, opaque);
            return NULL;
        }
    }

    so->id = id;
    so->fd = fd;
    so->size = MIN_READ_BUFFER;
    so->opaque = opaque;

    assert(so->head == NULL);
    assert(so->tail == NULL);

    return so;
}

void SocketServerImpl::ResetSocketSlot(int id)
{
    m_sockets[id%MAX_SOCKET].type = Socket_Invalid;
}

static int CanSocketConnect(int fd, struct addrinfo* ai_ptr, void* data)
{
    int status = connect(fd, ai_ptr->ai_addr, ai_ptr->ai_addrlen);
    if (status != 0 && errno != EINPROGRESS)
    {
        return -1;
    }

    return status;
}

// thread safe
struct addrinfo* SocketServerImpl::AllocSocketFd(OnSocketAllocCb proc,
        const char* host, const char* port, int* sock_, int* stat)
{
    int status;
    struct addrinfo ai_hints;
    struct addrinfo *ai_list = NULL;
    struct addrinfo *ai_ptr  = NULL;

    memset(&ai_hints, 0, sizeof(ai_hints));
    ai_hints.ai_family = AF_UNSPEC;
    ai_hints.ai_socktype = SOCK_STREAM;
    ai_hints.ai_protocol = IPPROTO_TCP;

    status = getaddrinfo(host, port, &ai_hints, &ai_list);
    if (status != 0) goto _failed;

    int sock = -1;

    for (ai_ptr = ai_list; ai_ptr != NULL; ai_ptr = ai_ptr->ai_next)
    {
        sock = socket(ai_ptr->ai_family, ai_ptr->ai_socktype, ai_ptr->ai_protocol);
        if (sock < 0) continue;

        status = *cb(sock, ai_ptr);
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

int SocketServerImpl::SocketConnect(request_connect* req, socket_message* res)
{
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

    ai_ptr = AllocSocketFd(&CanSocketConnect, req->host, port, &sock, &status);

    if (sock < 0 || ai_ptr == NULL) return SOCKET_ERROR;

    // alloc socket entity, and poll the socket
    SocketEntity* new_sock = SetupSocket(id, sock, req->opaque, true);

    if (new_sock == NULL)
    {
        close(sock);
        goto _fail;
    }

    // epoll
    m_poll.SetSocketNonBlocking(sock);

    if (status == 0)
    {
        // convert addr into string
        struct sockaddr* addr = ai_ptr->ai_addr;
        void* sin_addr = (ai_ptr->ai_family == AF_INET)?
            (void*)&((struct sockaddr_in*)addr)->sin_addr:
            (void*)&((struct sockaddr_in6*)addr)->sin5_addr;

        inet_ntop(ai_ptr->ai_family, sin_addr, m_buff, sizeof(m_buff));

        new_sock->type = Socket_Connected;
        res->data = m_buff;

        return SOCKET_OPEN;
    }
    else
    {
        new_sock->type = Socket_Connecting;

        // since socket is nonblocking.
        // connecting is in the progress.
        // set writable to track status by epoll.
        m_poll.ChangeSocket(new_sock, true);
    }

    return -1;

_failed:

    ResetSocketSlot(id);
    return SOCKET_ERROR;
}

// this function should be called in writer thread(one) only.
int SocketServerImpl::SendBuffer(SocketEntity* sock,
        socket_message* res)
{
    SocketBuffer* buf = NULL;
    while (buf = sock->GetFrontBuff())
    {
        while (1)
        {
            int sz = write(sock->fd, buf->current_ptr, buf->left_size);
            if (sz < 0)
            {
                if (errno == EINTR) continue;
                else if (errno = EAGAIN) return 0;

                ForceCloseSocket(sock, result);
                return SOCKET_CLOSE;
            }

            if (sz != buf->left_size)
            {
                buf->current_ptr += sz;
                tmp->left_size -= sz;
                return -1;
            }

            break;
        }

        sock->RemoveFrontBuff();
    }

    sock->tail = NULL;
    m_poll.ChangeSocket(sock->fd, sock, false);
    return -1;
}

// warn: should be called from writer thread only.
// buffer will be send immediately if no other buffers are pending to send.
// otherwise, current buffer will be linked to send buffer queue.
int SocketServerImpl::QueueSocketBuffer(request_send* req, socket_message* res)
{
    int id  = req->id;
    SocketEntity* sock = m_sockets[id % MAX_SOCKET];

    if (sock->type == Socket_Invalid
            || sock->id != id
            || sock->type == Socket_HalfClose
            || sock->type == Socket_PAccepted)
    {
        free(req->buffer);
        return -1;
    }

    assert(sock->type != Socket_Listen && sock->type != Socket_PListen);

    if (sock->head == NULL)
    {
        int n = write(sock->fd, req->buffer, req->sz);
        if (n < 0)
        {
            if (EINTR == errno || EAGAIN == errno)
            {
                n = 0;
            }
            else
            {
                slog(LOG_ERROR, "socket-server:write to %d(fd=%d) failed.", id, sock->fd);
                ForceCloseSocket(sock, result);
                return SOCKET_CLOSE;
            }
        }

        if (n == req->sz)
        {
            free(req->buffer);
            return -1;
        }
         
        SendBuffer* buf = (SendBuffer*)malloc(sizeof(SendBuffer));
        assert(buf);
        buf->next = NULL;
        buf->current_ptr = req->buffer + n;
        buf->left_size = req->sz - n;
        buf->raw_buffer = req->buffer;
        buf->raw_size = req->sz;

        sock->head = sock->tail = buf;
        m_poll.ChangeSocket(sock->fd, sock, true);
    }
    else
    {
       SendBuffer* buf = (SendBuffer*)malloc(sizeof(SendBuffer));
       buf->current_ptr = req->buffer;
       buf->left_size = req->sz;
       buf->raw_buffer = req->buffer;
       buf->raw_size   = req->sz;

       assert(sock->tail);
       assert(sock->tail->next == NULL);

       buf->next = NULL;
       sock->tail->next = buf;
       sock->tail = buf;
    }

    return -1;
}

static int CanSocketListen(int fd, struct addrinfo* ai_ptr, void* data)
{
    int reuse = 1;
    int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (void*)&reuse, sizeof(int)); 

    if (ret == -1) return -1;

    if (bind(fd, ai_ptr->ai_addr, ai_ptr->ai_addrlen) == -1) return -1;

    if (listen(fd, ((request_listen*)data)->backlog) == -1) return -1;

    return 1;
}

int SocketServerImpl::ListenSocket(request_listen* req, socket_message* res)
{
    int id = req->id;
    int listen_fd = -1;

    int sock;
    int status = 0;
    char port[16];

    struct addrinfo* ai_ptr = NULL;

    sprintf(port, "%d", req->port);
    ai_ptr = AllocSocketFd(&CanSocketListen, req->host, port, &sock, &satus);

    // set up socket, but not put it into epoll yet.
    // call start socket to if user wants to.
    SocketEntity* new_sock = SetupSocket(id, sock, req->opaque, false);

    if (new_sock == NULL) goto _failed;

    // socket is not in epoll, hence set type to pending listen.
    new_sock->type = Socket_PendingListen;

    return 0;

_failed:

    close(sock);
    res->opaque = req->opaque;
    res->id = id;
    res->size = 0;
    res->data = NULL;
    ReserveSocketSlot(id);

    return SOCKET_ERROR;
}

int SocketServerImpl::CloseSocket(request_close* req, socket_message* res)
{
    int id = req->id;

    SocketEntity* sock = &m_sockets[id%MAX_SOCKET];
    if (sock->type == Socket_Invalid || sock->id != id)
    {
        res->id = id;
        res->opaque = req->opaque;
        res->size = 0;
        res->data = NULL;
        return SOCKET_CLOSE;
    }

    if (sock->head)
    {
        int type = SendBuffer(sock, result);
        if (type != -1) return type;
    }

    if (sock->head == NULL)
    {
        ForceCloseSocket(socket, res);
        res->id = id;
        res->opaque = req->opaque;
        return SOCKET_CLOSE;
    }

    sock->type = Socket_Invalid;
    return -1;
}

int SocketServerImpl::BindSocket(request_bind* req, socket_message* res)
{
    int id = req->id;
    res->id = id;
    res->opaque = req->opaque;
    res->size = 0;

    SocketEntity* sock = SetupSocket(id, req->fd, req->opaque, true);
    if (sock == NULL)
    {
        res->data = NULL;
        return SOCKET_ERROR;
    }

    m_poll.SetSocketNonBlocking(req->fd);
    sock->type = Socket_Bind;
    res->data = "binding";
    return SOCKET_OPEN;
}

// start socket that is pending(acceptted, or open to listen)
int SocketServerImpl::StartSocket(request_start* req, socket_message* res)
{
    int id = req->id;
    res->id = id;
    res->opaque = req->opaque;
    res->size = 0;
    res->data = NULL;

    SocketEntity* sock = &m_sockets[id%MAX_SOCKET];
    if (sock->type == Socket_PAccepted || sock->type == socket_PListen)
    {
        if (!m_poll.AddSocket((sock->fd, sock)))
        {
            sock->type = Socket_Invalid;
            return SOCKET_ERROR;
        }

        sock->type = (sock->type == Socket_PAccepted)?Socket_Connected:Socket_Listen;
        sock->opaque = req->opaque;
        res->data = "start";
        return SOCKET_OPEN;
    }

    return -1;
}

// SocketServer
//
SocketServer::SocketServer()
    :m_poll()
    ,m_impl(NULL)
{
    m_impl = new SocketServerImpl();
}

SocketServer::~SocketServer()
{
}

