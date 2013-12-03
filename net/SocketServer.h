#ifndef __SOCKET_SERVER_H__
#define __SOCKET_SERVER_H__

#include "misc/NonCopyable.h"
#include <stdint.h>

/*
 * server is based on epoll.
 * internally, there will be an isolated thread polling the socket r/w event.
 */

struct SocketMessage
{
    int   id;    // socket id associated with the event.
    int   opaque;

    // for send/halfsend buffer event, ud == buffer size that is sended.
    // for read complete event, ud == size of read buffer.
    // for socket accept event, this the socket that is accepted.
    int   ud;

    // for socket connected event, this is the hostname of the connected socket.
    // for read socket event, this the data received, size is denoted by ud.
    const char* data;
};

enum SocketCode
{
    SC_NONE,
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

typedef void (*SocketEventHandler)(SocketCode, SocketMessage*);

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
        // param 'copy', control whether we should copy the buff.
        // if copy == false, please make sure, the buff is allocated from malloc()
        // because after the buffer is sended, server will freed manually.
        int SendBuffer(int id, const void* buff, int sz, bool copy = false);

        int SendString(int id, const char* buff);

        // add the corresponding socket to be watched.
        void WatchSocket(int id, uintptr_t opaque);

        int WatchRawSocket(int fd, uintptr_t opaque);

        // handler will be called in the polling thread whenever I/O is ready, make sure it is thread safe.
        // and since the polling thread is responsible for all socket I/O events, it is also critical
        // to make sure that handler is fast as possible
        void RegisterSocketEventHandler(SocketEventHandler handler);

    private:

        ServerImpl* m_impl;
};

#endif

