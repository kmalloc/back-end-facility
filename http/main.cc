#include "HttpServer.h"

#include "sys/Log.h"

#include <unistd.h>
#include <iostream>
using namespace std;

static void WorkerProc(int fd)
{
    InitLogger();

    HttpServer* server = new HttpServer();

    server->SetListenSock(fd);
    server->RunServer();

    delete server;
}

int main(int argc, char* argv[])
{
    if (argc <= 1)
    {
        cout << "Please specify addr to listen to" << endl;
        return 0;
    }

    const char* addr = argv[1];

    int port = 80;

    if (argc >= 3) port = atoi(argv[2]);

    if (argc >= 4) SetLogLevel(atoi(argv[3]));

    int fd = ListenTo(addr, port);

    if (fd < 0)
    {
        cout << "failed to listen to " << addr << ":" << port << endl;
        return 0;
    }

    int i = 0;
    int num = sysconf(_SC_NPROCESSORS_CONF);

    while (i < num)
    {
        int pid = fork();
        if (pid == 0) break;

        ++i;
    }

    if (i < num) WorkerProc(fd);

    char c;
    cin >> c;
    return 0;
}

