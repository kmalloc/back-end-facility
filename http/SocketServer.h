#ifndef __SOCKET_SERVER_H__
#define __SOCKET_SERVER_H__

#include "misc/functor.h"
#include "misc/NonCopyable.h"
#include <stdint.h>

int ListenTo(const char* host, int _port);

class ServerImpl;
class SocketServer;

enum SocketCode
{
    SC_READ, // socket is ready to read data, SocketEvent::fd denotes corresponding fd, note: corresponding fd will be removed poller
    SC_WRITE, // socket is ready to send data, note: corresponding fd is removed from poller.
    SC_CONNECTED, // SocketEvent::fd denotes the corresponding socket
    SC_ACCEPTED, // 
    SC_FAIL_CONN, // fail to connect, need to close socket.
    SC_ERROR,  // out of resource: socket fd or memory

    SC_SUCC
};

class SocketConnection: public noncopyable
{
    public:

        SocketConnection(ServerImpl* server = 0);
        ~SocketConnection();

        int SendBuffer(const char* buff, int sz);
        int ReadBuffer(char* buff, int sz);

        void CloseConnection();

        bool IsConnected() const;

        uintptr_t GetOpaqueValue() const;

        void SetServerImpl(ServerImpl* server) { server_ = server; }

        int GetConnectionId() const;

    private:

        ServerImpl* server_;
};

struct SocketEvent
{
    SocketCode code;
    SocketConnection* conn;
};

class SocketServer: public noncopyable
{
    public:

        SocketServer();
        ~SocketServer();

        // start server
        void StartServer();

        // stop server.
        // close all sockets
        void StopServer();

        void RunPoll(SocketEvent* evt);

        int ListenTo(const char* ip, int port, uintptr_t opaque = 0);

        // connect to a remote host
        SocketConnection* ConnectTo(const char* ip, int port, uintptr_t opaque = 0);

        // add the corresponding socket to be watched.
        bool WatchSocket(int fd, bool listen = false);

        // by default, newly accepted socket is not watched by epoll.
        // call SetWatchAcceptedSock() to change this behavior.
        // function is not thread safe.
        void SetWatchAcceptedSock(bool watch);

    public:

        static const int max_conn_id;

    private:

        friend class SocketConnection;

        bool CloseSocket(int fd);
        int SendBuffer(int fd, const char* buff, int sz);
        int ReadBuffer(int fd, char* data, int sz);

        ServerImpl* impl_;
};

#endif
