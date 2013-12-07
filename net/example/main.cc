#include "net/SocketServer.h"

#include "sys/AtomicOps.h"

#include <string.h>
#include <stdlib.h>

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

static void handler(SocketCode code, SocketMessage* msg)
{
    switch (code)
    {
        case SC_EXIT:
            return;
        case SC_CLOSE:
            cout << "socket(" << msg->id << ") is closed" << endl;
            break;
        case SC_HALFCLOSE:
            cout << "socket(" << msg->id << ") is closing, need to send out pending buffer first" << endl;
            break;
        case SC_CONNECTED:
            cout << "socket(" << msg->id << ") connected, operation id:" << msg->opaque << endl;
            break;
        case SC_DATA:
            cout << "socket(" << msg->id << ") receive data:" << (char*)msg->data << ", size:" << msg->ud << endl;
            if (memcmp(msg->data, "py test client:", 15) == 0)
            {
                vector<string> out;
                Split(msg->data, ':', out);
                server.Connect(out[1].c_str(), atoi(out[2].c_str()), g_op);
                atomic_increment(&g_op);

                server.SendBuffer(msg->id, msg->data, msg->ud, true);
            }
            else if (memcmp(msg->data, "wewe:own", 8))
            {
                server.SendBuffer(msg->id, msg->data, msg->ud, true);
            }

            free(msg->data);
            break;
        case SC_ACCEPT:
            cout << "socket(" << msg->ud << ") accepted, from server id: " << msg->id << endl;
            server.WatchSocket(msg->ud, g_op);
            atomic_increment(&g_op);
            break;
        case SC_SEND:
            cout << "socket(" << msg->id << ") send buffer done:" << msg->ud << endl;
            break;
        case SC_HALFSEND:
            cout << "socket(" << msg->id << ") sending buffer, buffer send:"
                << msg->ud << "bytes, not finish yet" << endl;
            break;
        case SC_LISTEN:
            cout << "socket(" << msg->id << ") listen, operation id:" << msg->opaque << endl;
            break;
        case SC_WATCHED:
            cout << "socket(" << msg->id << ") is added to be watched, operation:" << msg->opaque << endl;
            break;
        default:
            cout << "enter handler, code:" << code << ", id:" << msg->id << ", opaque:" << msg->opaque << endl;
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

    server.StartServer(host, port, g_op);
    atomic_increment(&g_op);

    int id = server.Connect(host, port, g_op);
    atomic_increment(&g_op);

    sleep(1);
    server.SendString(id, "wewe:own,12345678");

    int id2 = server.Connect(host, port, g_op);
    atomic_increment(&g_op);

    server.ListenTo(host, port + 1, g_op);
    atomic_increment(&g_op);
    server.ListenTo(host, port - 1, g_op);
    atomic_increment(&g_op);

    sleep(1);

    server.ListenTo(host, port + 2, g_op);
    atomic_increment(&g_op);
    server.ListenTo(host, port - 2, g_op);
    atomic_increment(&g_op);

    int id3 = server.Connect(host, port + 1, g_op);
    atomic_increment(&g_op);
    int id4 = server.Connect(host, port - 1, g_op);
    atomic_increment(&g_op);

    sleep(1);

    int id5 = server.Connect(host, port + 2, g_op);
    atomic_increment(&g_op);
    int id6 = server.Connect(host, port - 2, g_op);
    atomic_increment(&g_op);

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
    server.CloseSocket(id4, g_op);
    atomic_increment(&g_op);

    int i;
    cin >> i;
    return 0;
}


