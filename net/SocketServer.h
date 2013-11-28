#ifndef __SOCKET_SERVER_H__
#define __SOCKET_SERVER_H__

#include "misc/NonCopyable.h"


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
    int   id;
    int   opaque;
    int   ud;
    const char* data;
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
        int StartListen(const char* ip, int port);

        // connect to a remote host
        // return value is the connected socket id on success.
        // return -1 on error.
        int Connect(const char* ip, int port);

        // send buffer to socket identified by id.
        int SendBuffer(int id, void* buff, int sz);

        // close the connected socket identified by id.
        int CloseSocket(int id);

        // stop server.
        // a) stop the polling thread.
        // b) sockets are not close.
        int StopServer();

    protected:

        // socket events
        // b) new socket accepted.
        // c) socket connected.
        // d) data read.
        // c) socket error.
        // e) socket close.
        //

    private:

        ServerImpl* m_impl;
};

#endif

