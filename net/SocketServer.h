#ifndef __SOCKET_SERVER_H__
#define __SOCKET_SERVER_H__

#include "misc/NonCopyable.h"

struct SocketMessage
{
    int   socketid;
    int   identifier;
    int   size;
    void* data;
};

class SocketServer: public noncopyable
{
    public:

        SocketServer();
        ~SocketServer();

        // start server
        // a) start polling thread.
        // b) setup socket pool.
        int StartListen(const std::string& ip, int port);

        // connect to a remote host
        // return value is the connected socket id on success.
        // return -1 on error.
        int Connect(const std::string& ip, int port);

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
        // whatever event arrives, this callback is supposed to be really lightweight.
        // don't do time consuming task within it.
        // best practice is to post task to other thread to handle this socket event if necessary.
        virtual int OnSocketEvent(int id, SocketMessage msg) = 0;

    private:
    
        struct SocketServerImpl;

        SocketServerImpl* m_impl;
};

#endif

