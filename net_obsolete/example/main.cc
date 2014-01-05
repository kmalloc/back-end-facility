#include "net/SocketServer.h"
#include "net/SocketBuffer.h"

#include "sys/AtomicOps.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <set>
#include <vector>
#include <sstream>
#include <iostream>

using namespace std;

static int g_op = 0;
SocketServer server;

static int Split(const string& str, char delim, vector<string>& out)
{
    int c = 0;

    string item;
    stringstream ss(str);

    while (getline(ss, item, delim))
    {
        ++c;
        out.push_back(item);
    }

    return c;
}

struct SockConn
{
    void* data;
    char  stream[256];
    SocketBufferList buff;
};

static SockConn g_conn[65536];

static set<int> g_stop_sock;
static set<int> g_stop_code;


static void ClearPendingBuffer(int sock)
{
    SockConn* conn = &g_conn[sock];

    SocketBufferNode* node;

    while ((node = conn->buff.PopFrontNode()))
    {
        conn->buff.FreeBufferNode(node);
    }
}

static int SendPendingBuffer(int sock)
{
    int ret = 0;
    SockConn* conn = &g_conn[sock];

    while (1)
    {
        SocketBufferNode* node = conn->buff.GetFrontNode();
        if (node == NULL) break;

        int sz = server.SendBuffer(sock, node->curPtr_, node->curSize_, true);

        ret += sz;

        if (sz == node->curSize_)
        {
            conn->buff.PopFrontNode();
            conn->buff.FreeBufferNode(node);
        }
        else
        {
            node->curPtr_ += sz;
            node->curSize_ -= sz;
            break;
        }
    }

    return ret;
}


static void SendBuffer(int sock, const char* buff, int sz)
{
    SocketBufferNode* node = g_conn[sock].buff.AllocNode(sz);
    assert(node);

    node->curSize_ = sz + 1;
    memcpy(node->curPtr_, buff, sz + 1);
    g_conn[sock].buff.AppendBufferNode(node);

    SendPendingBuffer(sock);
}

static void handle_data(int sock, const char* txt, int size)
{
    cout << "socket(" << sock << ") receive data:" << txt << ", size:" << size << endl;

    set<int>::const_iterator it = g_stop_sock.find(sock);
    if (it != g_stop_sock.end())
    {
        char* info = "stop listening now!";
        int sz = strlen(info) + 1;

        SendBuffer(sock, info, sz);

        cout << "socket(sock) send stop signal to client" << endl;

        g_stop_sock.erase(it);
    }
    else if (memcmp(txt, "py test client:", 15) == 0)
    {
        vector<string> out;
        Split(txt, ':', out);

        int op_code = atomic_increment(&g_op);
        if (out[3] == "no connect")
        {
            g_stop_code.insert(op_code);
            cout << "receive close req:op code: " << endl;
        }

        server.ConnectTo(out[1].c_str(), atoi(out[2].c_str()), op_code);
        // echo msg
        SendBuffer(sock, txt, size);
    }
    else if (memcmp(txt, "wewe:own", 8))
    {
        SendBuffer(sock, txt, size);
    }
}

static const int READ_WRITE_SZ = 512;

static void handler(SocketEvent evt)
{
    SocketCode code = evt.code;
    SocketMessage msg = evt.msg;

    switch (code)
    {
        case SC_EXIT:
            return;
        case SC_CLOSE:
            ClearPendingBuffer(msg.fd);
            cout << "socket(" << msg.fd << ") is closed" << endl;
            break;
        case SC_CONNECTED:
            {
                set<int>::const_iterator it = g_stop_code.find(msg.opaque);

                if (it != g_stop_code.end())
                {
                    g_stop_code.erase(it);
                    g_stop_sock.insert(msg.fd);
                }

                cout << "socket(" << msg.fd << ") connected, operation id:" << msg.opaque << endl;
            }
            break;
        case SC_WRITEREADY:
            {
                SendPendingBuffer(msg.fd);
            }
            break;
        case SC_READREADY:
            {
                // user better define msg format, including in some fields to indicate the package size.
                // so that we can elimite the following string operation which can be very time consuming.

                char* data = (char*)malloc(READ_WRITE_SZ + 2);
                assert(data);

                data[READ_WRITE_SZ] = 0xf8;
                data[READ_WRITE_SZ + 1] = 0x8f;

                int ret = server.ReadBuffer(msg.fd, data, READ_WRITE_SZ, true);

                if (ret <= 0)
                {
                    free(data);
                    break;
                }

                int sz = strlen(data) + 1;
                int left = strlen(g_conn[msg.fd].stream);

                if (sz > ret)
                {
                    // in this case, package partially received.
                    memcpy(g_conn[msg.fd].stream + left, data, ret);
                    g_conn[msg.fd].stream[left + ret] = 0;
                    break;
                }

                int size = left + sz;

                strncat(g_conn[msg.fd].stream, data, sz);

                char* txt = g_conn[msg.fd].stream;

                handle_data(msg.fd, txt, size);

                // store the msg that is partial received.
                left = ret - sz;
                assert(left >= 0);

                while (left)
                {
                    int end = strlen(data + sz) + 1;

                    if (end <= left)
                    {
                        handle_data(msg.fd, data + sz, end);
                        left -= end;
                        sz += end;
                    }
                    else
                    {
                        memcpy(g_conn[msg.fd].stream, data + sz, left);
                        break;
                    }
                }

                free(data);
                g_conn[msg.fd].stream[left] = 0;
                break;
            }
        case SC_ACCEPTED:
            {
                cout << "socket(" << msg.ud << ") accepted, from server id: " << msg.fd << endl;

                int op = atomic_increment(&g_op);
                server.WatchSocket(msg.ud, op);
            }
            break;
        case SC_LISTENED:
            cout << "socket(" << msg.fd << ") listen, operation id:" << msg.opaque << endl;
            break;
        case SC_WATCHED:
            cout << "socket(" << msg.fd << ") is added to be watched, operation:" << msg.opaque << endl;
            break;
        default:
            cout << "enter handler, code:" << code << ", id:" << msg.fd << ", opaque:" << msg.opaque << endl;
            break;
    }
}

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        cout << "please specify addr && port to listen to\n";
        return 0;
    }

    char* host = argv[1];
    int   port = atoi(argv[2]);

    cout << "server main." << endl;

    server.RegisterSocketEventHandler(handler);

    int op = atomic_increment(&g_op);
    server.StartServer(host, port, op);

    op = atomic_increment(&g_op);
    server.ConnectTo(host, port, op);

    sleep(1);

    op = atomic_increment(&g_op);
    server.ConnectTo(host, port, op);

    op = atomic_increment(&g_op);
    server.ListenTo(host, port + 1, op);

    op = atomic_increment(&g_op);
    server.ListenTo(host, port - 1, op);

    sleep(1);

    op = atomic_increment(&g_op);
    server.ListenTo(host, port + 2, op);

    op = atomic_increment(&g_op);
    server.ListenTo(host, port - 2, op);

    op = atomic_increment(&g_op);
    server.ConnectTo(host, port + 1, op);

    op = atomic_increment(&g_op);
    server.ConnectTo(host, port - 1, op);

    sleep(1);

    op = atomic_increment(&g_op);
    server.ConnectTo(host, port + 2, op);

    op = atomic_increment(&g_op);
    server.ConnectTo(host, port - 2, op);

    sleep(1);

    int i;
    cin >> i;
    return 0;
}


