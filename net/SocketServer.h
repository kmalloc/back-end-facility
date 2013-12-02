#ifndef __SOCKET_SERVER_H__
#define __SOCKET_SERVER_H__

#include "misc/NonCopyable.h"
#include <stdint.h>

/*
 * whole server is based on epoll.
 * internally, there will be 3 threads running for the server.
 * 1) poll thread, waiting on epoll_wait().
 *    this thread handle all the lightweight event.
 *    and dispatch event to other 2 threads.
 * 2) writer thread, this thread is responsible for sending buffer for ALL sockets
 *    when write is possible on any of them. And, all send operations are nonblocking.
 *
 * 3) reader thread, this thread is responsible for receiving data for ALL sockets when read
 *    is possible on any socket.
 *
 */

struct SocketMessage
{
    int   id; // socket id associated with the event.
    int   opaque;

    // for send/halfsend buffer event, ud == buffer size that is sended.
    // for read complete event, ud == size of read buffer.
    // for socket accept event, this the socket that is accepted.
    int   ud;

    // for socket connected event, this is the hostname of the connected socket.
    // for read socket event, this the data received, size is denoted by ud.
    const char* data;
};

typedef void (*SocketEventHandler)(int, SocketMessage*);

enum SocketCode
{
    SC_BADSOCK,
    SC_DATA,
    SC_CLOSE,
    SC_HALFCLOSE,
    SC_LISTEN,
    SC_CONNECTED,
    SC_ACCEPT,
    SC_HALFSEND,
    SC_SEND,
    SC_WATCHED, // socket is added to be watched
    SC_ERROR,
    SC_IERROR, // error ignored
    SC_EXIT,

    SC_SUCC
};

class ServerImpl;

class SocketServer: public noncopyable
{
    public:

        SocketServer();
        ~SocketServer();

        // start server
        // a) start polling thread.
        // b) setup socket pool.
        int StartServer(const char* ip, int port, uintptr_t opaque = -1);

        // stop server.
        // a) stop the polling thread.
        // b) sockets are not close.
        void StopServer();

        int ListenTo(const char* ip, int port, uintptr_t opaque = -1);

        // connect to a remote host
        // return value is the connected socket id on success.
        // return -1 on error.
        int Connect(const char* ip, int port, uintptr_t opaque = -1);

        // close the connected socket identified by id.
        void CloseSocket(int id, uintptr_t opaque = -1);

        // send buffer to socket identified by id.
        int SendBuffer(int id, const void* buff, int sz);

        int SendString(int id, const char* buff);

        void WatchSocket(int id, uintptr_t opaque);

        int WatchRawSocket(int fd, uintptr_t opaque);

        /*
         * switch (code)
         * {
         *     case SC_EXIT:
         *         return;
         *     case SC_CLOSE:
         *         break;
         *     case SC_HALFCLOSE:
         *         break;
         *     case SC_CONNECTED:
         *         break;
         *     case SC_DATA:
         *         break;
         *     case SC_ACCEPT:
         *         break;
         *     case SC_SEND:
         *         break;
         *     case SC_HALFSEND:
         *         break;
         *     default:
         *         break;
         * }
         */

        void RegisterSocketEventHandler(SocketEventHandler handler);

    protected:

    private:

        ServerImpl* m_impl;
};

#endif

