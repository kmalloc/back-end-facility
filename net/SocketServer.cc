#include "SocketServer.h"

#include "sys/Log.h"
#include "sys/AtomicOps.h"
#include "misc/SocketPoll.h"
#include "thread/Thread.h"

#define MAX_SOCKET (1 << sizeof(short))
#define MIN_READ_BUFFER (64)

enum SocketStatus
{
    Socket_Invalid,
    Socket_Reserved,
    Socket_Listen,
    Socket_PendingListen,
    Socket_Connected,
    Socket_Connecting,
    Socket_Accepted
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
    uintptr_t identifier;
    int size; // fix size for each read operation.
    
    // buffer that is pending to send.
    SocketBuffer* head;
    SocketBuffer* tail;
};

struct request_connect
{
    int id;
    int port;
    uintptr_t identifier;

    char host[1];
};

struct request_send
{
    int id;
    int sz;
    uintptr_t identifier;
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
    uintptr_t identifier;

    char host[1];
};

struct request_start
{
    int id;
    uintptr_t identifier;
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

        friend class SocketServer;

    protected:

        int  Connect(const string& ip, int port);
        int  StartServer();
        int  StartListen(const string& ip, int port);
        int  SendBuffer(int id, void* data, int len);
        void CloseSocket(SocketEntity* sock, SocketMessage* result);
        void ForceCloseSocket(SocketEntity* so, SocketMessage* res);
        void SetupSocket(int id, int fd, uintptr_t identifier, bool poll); 

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

SocketServerImpl::~SocketServerImpl()
{
}

int SocketServerImpl::ReserveSocketSlot()
{
    for (int i = 0; i < MAX_SOCKET; ++i)
    {
        int id = atomic_increment(&m_allocId);
        if (id < 0) id = atomic_and_and_fetch(&m_allocId, ~(1 << (sizeof(int) - 1)));

        struct SocketEntity* sock = &m_sockets[id%MAX_SOCKET]; 

        if (sock->type == Socket_Invalid)
        {
            if (atomic_cas(&sock->type, Socket_Invalid, Socket_Reserved))
                return id;

            --i;
        }
    }

    return -1;
}

// SocketServerImpl
//
SocketServerImpl::SocketServerImpl()
    :m_pollEventIndex(0)
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
    result->identifier = sock->identifier;
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
        uintptr_t identifier, bool poll)
{
    SocketEntity* so = &m_sockets[id % MAX_SOCKET];
    assert(so->type == Socket_Reserved);

    if (poll)
    {
        if (!m_poll.AddSocket(fd, so))
        {
            so->type = Socket_Invalid;
            slog(LOG_ERROR, "SetupSocket failed, id: %d, identifier:%d", id, identifier);
            return NULL;
        }
    }

    so->id = id;
    so->fd = fd;
    so->size = MIN_READ_BUFFER;
    so->identifier = identifier;

    assert(so->head == NULL);
    assert(so->tail == NULL);

    return so;
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

