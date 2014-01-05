#include "SocketBuffer.h"

#include <stdlib.h>
#include <assert.h>


SocketBufferList::SocketBufferList()
    :head(NULL), tail(NULL)
{
}

SocketBufferList::~SocketBufferList()
{
    while (1)
    {
        SocketBufferNode* node = PopFrontNode();
        if (node == NULL) break;

        FreeBufferNode(node);
    }
}

SocketBufferNode* SocketBufferList::AllocNode(int size)
{
    SocketBufferNode* node = (SocketBufferNode*)malloc(size + sizeof(SocketBufferNode));
    if (node)
    {
        node->size_ = size;
        node->curSize_ = 0;
        node->curPtr_ = node->memFrame_;
        node->next_ = NULL;
    }

    return node;
}

void SocketBufferList::FreeBufferNode(SocketBufferNode* node)
{
    free(node);
}

SocketBufferNode* SocketBufferList::PopFrontNode()
{
    SocketBufferNode* ret = head;

    if (head)
    {
        head = head->next_;
    }

    if (head == NULL) tail = NULL;

    return ret;
}

SocketBufferNode* SocketBufferList::GetFrontNode() const
{
    return head;
}

void SocketBufferList::AppendBufferNode(SocketBufferNode* node)
{
    assert(node);

    if (tail) tail->next_ = node;

    if (head == NULL) head = node;

    tail = node;
}

