#include "SocketServer.h"

#include <string.h>
#include <stdlib.h>
#include <iostream>
using namespace std;

SocketServer server;

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
            cout << "socket is closing, need to send out pending buffer first, id: " << msg->id << endl;
            break;
        case SC_CONNECTED:
            cout << "socket connected, id:" << msg->id << ", opaque:" << msg->opaque << endl;
            break;
        case SC_DATA:
            cout << "receive data from socket:" << msg->id << ", data:" << (char*)msg->data << ", data size:" << msg->ud << endl;
            free(msg->data);
            break;
        case SC_ACCEPT:
            server.WatchSocket(msg->ud, 12333);
            cout << "socket accepted, id:" << msg->ud << ", from server id: " << msg->id << endl;
            break;
        case SC_SEND:
            cout << "sock:" << msg->id << ", buffer send:" << msg->ud << endl;
            break;
        case SC_HALFSEND:
            cout << "sock:" << msg->id << "send buffer is in progress, buffer send:"
                << msg->ud << ", not finish yet" << endl;
            break;
        case SC_LISTEN:
            cout << "listen done, socket id:" << msg->id << ", opaque:" << msg->opaque << endl;
            break;
        case SC_WATCHED:
            cout << "socket:" << msg->id << " is added to be watched, opaque:" << msg->opaque << endl;
            break;
        default:
            cout << "enter handler, code:" << code << ", id:" << msg->id << ", opaque:" << msg->opaque << endl;
            break;
    }
}

int main()
{
    cout << "server main." << endl;

    server.RegisterSocketEventHandler(handler);

    server.StartServer("127.0.0.1", 1388, 12306);

    int id = server.Connect("127.0.0.1", 1388, 1);

    sleep(1);
    server.SendString(id, "12345678");

    int id2 = server.Connect("127.0.0.1", 1388, 2);
    server.ListenTo("127.0.0.1", 1389, 23);
    server.ListenTo("127.0.0.1", 1387, 24);

    sleep(1);

    server.ListenTo("127.0.0.1", 1399, 25);
    server.ListenTo("127.0.0.1", 1397, 26);

    int id3 = server.Connect("127.0.0.1", 1389, 11);
    int id4 = server.Connect("127.0.0.1", 1387, 12);

    sleep(1);

    int id5 = server.Connect("127.0.0.1", 1399, 13);
    int id6 = server.Connect("127.0.0.1", 1397, 14);

    sleep(1);

    void* data1 = malloc(24);
    memcpy(data1, &server, 24);

    server.SendBuffer(id3, data1, 24);
    server.SendString(id3, "hellohello");
    server.SendString(id4, "hellohello");
    server.SendString(id5, "hellohello");

    void* data2 = malloc(24);
    memcpy(data2, &server, 24);

    server.SendBuffer(id6, data2, 24);

    sleep(1);
    server.CloseSocket(id4, 44);

    sleep(50);
    return 0;
}


