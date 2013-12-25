#include "net/SocketServer.h"

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
};

static SockConn g_conn[65536];

static set<int> g_stop_sock;
static set<int> g_stop_code;

static void handle_data(int sock, const char* txt, int size)
{
    cout << "socket(" << sock << ") receive data:" << txt << ", size:" << size << endl;

    set<int>::const_iterator it = g_stop_sock.find(sock);
    if (it != g_stop_sock.end())
    {
        char info[] = "stop listening now!";
        cout << "socket(sock) send stop signal to client" << endl;
        server.SendBuffer(sock, info, sizeof(info), true);

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

        server.Connect(out[1].c_str(), atoi(out[2].c_str()), op_code);
        // echo msg
        server.SendBuffer(sock, txt, size, true);
    }
    else if (memcmp(txt, "wewe:own", 8))
    {
        server.SendBuffer(sock, txt, size, true);
    }
}

static void handler(SocketEvent evt)
{
    SocketCode code = evt.code;
    SocketMessage msg = evt.msg;

    switch (code)
    {
        case SC_EXIT:
            return;
        case SC_CLOSE:
            cout << "socket(" << msg.fd << ") is closed" << endl;
            break;
        case SC_HALFCLOSE:
            cout << "socket(" << msg.fd << ") is closing, need to send out pending buffer first" << endl;
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
        case SC_DATA:
            {
                // user better define msg format, including in some fields to indicate the package size.
                // so that we can elimite the following string operation which can be very time consuming.
                int sz = strlen((char*)msg.data) + 1;
                int left = strlen(g_conn[msg.fd].stream);

                if (sz > msg.ud)
                {
                    // package partial received.
                    memcpy(g_conn[msg.fd].stream + left, msg.data, msg.ud);
                    g_conn[msg.fd].stream[left + msg.ud] = 0;
                    free(msg.data);
                    break;
                }

                int size = left + sz;

                strncat(g_conn[msg.fd].stream, msg.data, sz);

                char* txt = g_conn[msg.fd].stream;

                handle_data(msg.fd, txt, size);

                // store the msg that is partial received.
                left = msg.ud - sz;
                assert(left >= 0);

                while (left)
                {
                    int end = strlen((char*)msg.data + sz) + 1;

                    if (end <= left)
                    {
                        handle_data(msg.fd, (char*)msg.data + sz, end);
                        left -= end;
                        sz += end;
                    }
                    else
                    {
                        memcpy(g_conn[msg.fd].stream, msg.data + sz, left);
                        break;
                    }
                }

                g_conn[msg.fd].stream[left] = 0;

                free(msg.data);
                break;
            }
        case SC_ACCEPT:
            {
                cout << "socket(" << msg.ud << ") accepted, from server id: " << msg.fd << endl;

                int op = atomic_increment(&g_op);
                server.WatchSocket(msg.ud, op);
            }
            break;
        case SC_SEND:
            cout << "socket(" << msg.fd << ") send buffer done:" << msg.ud << endl;
            break;
        case SC_HALFSEND:
            cout << "socket(" << msg.fd << ") sending buffer, buffer send:"
                << msg.ud << "bytes, not finish yet" << endl;
            break;
        case SC_LISTEN:
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
    if (argc < 2)
    {
        cout << "please specify port to listen to\n";
        return 0;
    }

    int port = atoi(argv[1]);
    char* host = "127.0.0.1";

    cout << "server main." << endl;

    server.RegisterSocketEventHandler(handler);

    int op = atomic_increment(&g_op);
    server.StartServer(host, port, op);

    op = atomic_increment(&g_op);
    int id = server.Connect(host, port, op);

    sleep(1);
    server.SendString(id, "wewe:own,12345678");

    op = atomic_increment(&g_op);
    int id2 = server.Connect(host, port, op);

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
    int id3 = server.Connect(host, port + 1, op);

    op = atomic_increment(&g_op);
    int id4 = server.Connect(host, port - 1, op);

    sleep(1);

    op = atomic_increment(&g_op);
    int id5 = server.Connect(host, port + 2, op);

    op = atomic_increment(&g_op);
    int id6 = server.Connect(host, port - 2, op);

    sleep(1);

    void* data1 = malloc(24);
    memcpy(data1, "wewe:own, some random buffer string", 24);

    server.SendBuffer(id3, data1, 24);
    server.SendString(id3, "wewe:own,hellohello");
    server.SendString(id4, "wewe:own,hellohello");
    server.SendString(id5, "wewe:own, hellohello");

    void* data2 = malloc(24);
    memcpy(data2, "wewe:own, some random buffer", 24);

    server.SendBuffer(id6, data2, 24);

    sleep(1);
    op = atomic_increment(&g_op);
    server.CloseSocket(id4, op);

    int i;
    cin >> i;
    return 0;
}


