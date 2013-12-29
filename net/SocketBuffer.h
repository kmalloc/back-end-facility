#ifndef __SOCKET_BUFFER_H__
#define __SOCKET_BUFFER_H__

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

        SocketBufferNode* AllocNode(int size) const;
        SocketBufferNode* GetFrontNode() const;
        SocketBufferNode* PopFrontNode();
        void FreeBufferNode(SocketBufferNode* node) const;
        void AppendBufferNode(SocketBufferNode* node);
    private:
        SocketBufferNode* head;
        SocketBufferNode* tail;
};

#endif

