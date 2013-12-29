#ifndef __HTTP_TASK_H__
#define __HTTP_TASK_H__

#include "thread/ITask.h"
#include "misc/LockFreeList.h"
#include "misc/LockFreeBuffer.h"

#include "net/SocketServer.h"
#include "net/http/HttpContext.h"


// bind each connection to one thread.
class HttpTask: public ITask
{
    public:

        HttpTask(SocketServer* server, LockFreeBuffer& msgPool);
        ~HttpTask();

        bool PostSockMsg(SocketEvent* msg);

    private:

        virtual void Run();

        void ResetTask(int connid);

        // release only when connectioin is close
        void ReleaseTask();

    private:

        void ProcessSocketMessage(SocketEvent* msg);

        SocketServer* tcpServer_;
        HttpContext context_;
        LockFreeBuffer& msgPool_;

        // one http connection can only be handled in one thread
        // so need to queue the msg
        LockFreeListQueue sockMsgQueue_;
};


#endif

