#ifndef __SOCKET_BUFFER_H__
#define __SOCKET_BUFFER_H__

#define MINI_SOCKET_READ_SIZE (64)

struct SocketBufferNode
{
    int size_;
    int curSize_;
    char* curPtr_;
    SocketBufferNode* next_;

    char memFrame_[1];
};

class SocketBufferList
{
    public:
        SocketBufferList();
        ~SocketBufferList();

        SocketBufferNode* GetFrontNode() const;
        SocketBufferNode* PopFrontNode();
        void AppendBufferNode(SocketBufferNode* node);

        static void FreeBufferNode(SocketBufferNode* node);
        static SocketBufferNode* AllocNode(int size);
    private:
        SocketBufferNode* head;
        SocketBufferNode* tail;
};

#endif

