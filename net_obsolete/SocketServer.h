#ifndef __SOCKET_SERVER_H__
#define __SOCKET_SERVER_H__

#include "misc/functor.h"
#include "misc/NonCopyable.h"
#include <stdint.h>

/*
 * server is based on epoll.
 * internally, there will be an isolated thread polling the socket r/w event.
 */
struct SocketMessage
{
    int fd; // socket id associated with the event.

    // for socket accept event, this the socket that is accepted.
    int ud;

    // for socket connected event, this is the hostname of the connected socket.
    // for read socket event, this the data received, size is denoted by ud.
    char* data;
    uintptr_t   opaque;
};

enum SocketCode
{
    SC_CLOSE, // SocketEvent::fd is the socket that been close
    SC_READREADY, // socket is ready to read data, SocketEvent::fd denotes corresponding fd, note: corresponding fd will be removed poller
    SC_WRITEREADY, // socket is ready to send data, note: corresponding fd is removed from poller.
    SC_LISTENED, // SocketEvent::fd denotes the corresponding socket
    SC_CONNECTED, // SocketEvent::fd denotes the corresponding socket
    SC_ACCEPTED, // SocketEvent::ud denotes the accepted socket, SocketEvent::id denotes the corresponding socket that is listening.
    SC_WATCHED, // socket is added to be watched, SocketEvent::id is the corresponding socket
    SC_ERROR,  // out of resource: socket fd or memory
    SC_IERROR, // error ignored
    SC_EXIT,

    SC_SUCC
};

struct SocketEvent
{
    SocketCode code;
    SocketMessage msg;
};

class ServerImpl;
typedef misc::function<void, SocketEvent> SocketEventHandler;

class SocketServer: public noncopyable
{
    public:

        SocketServer();
        ~SocketServer();

        // start server
        // a) start polling thread.
        // b) setup socket pool.
        int StartServer(const char* ip, int port, uintptr_t opaque = 0);

        // stop server.
        // a) stop the polling thread.
        // b) sockets are not close.
        void StopServer(uintptr_t opaque = 0);

        int ListenTo(const char* ip, int port, uintptr_t opaque = 0);

        // connect to a remote host
        // return value is the connected socket id on success.
        // return -1 on error.
        int ConnectTo(const char* ip, int port, uintptr_t opaque = 0);

        // send buffer to socket identified by id.
        // return val denotes the size of data been send.
        // param "watch" decides whether to watch the socket when send is not finished.
        int SendBuffer(int id, const void* buff, int sz, bool watch = true);
        int ReadBuffer(int fd, void* data, int sz, bool watch = true);

        // close the connected socket identified by id.
        bool CloseSocket(int id);

        // add the corresponding socket to be watched.
        bool WatchSocket(int id);

        // by default, newly accepted socket is not watched by epoll.
        // call SetWatchAcceptedSock() to change this behavior.
        // function is not thread safe.
        void SetWatchAcceptedSock(bool watch);

        // handler will be called in the polling thread whenever I/O is ready, make sure it is thread safe.
        // and since the polling thread is responsible for all socket I/O events, it is also critical
        // to make sure that handler is fast as possible
        void RegisterSocketEventHandler(SocketEventHandler handler);

        static void DefaultSockEventHandler(SocketEvent msg);

    public:

        static const int max_conn_id;

    private:

        ServerImpl* impl_;
};

#endif

