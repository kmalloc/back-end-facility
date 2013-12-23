#ifndef __HTTP_TASK_H__
#define __HTTP_TASK_H__

#include "thread/ITask.h"
#include "misc/LockFreeList.h"

#include "net/SocketServer.h"
#include "net/http/HttpContext.h"

// bind each connection to one thread.
class HttpTask: public ITask
{
    public:

        HttpTask(HttpServer* server, LockFreeBuffer& alloc);
        ~HttpTask();

        void ResetTask(int connid);
        bool PostSockMsg(ConnMessage* msg);

        // release only when connectioin is close
        void ReleaseTask();

        // initial on socket connected.
        void InitConnection();

        virtual void Run();

        void ClearMsgQueue();

        void CloseTask();

    private:

        void ProcessHttpData(const char* data, size_t sz);
        void ProcessSocketMessage(ConnMessage* msg);

        bool taskClosed_;
        HttpServer* httpServer_;
        HttpContext context_;

        // one http connection can only be handled in one thread
        // so need to queue the msg
        LockFreeListQueue sockMsgQueue_;
};


#endif

