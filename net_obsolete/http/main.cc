#include "HttpServer.h"

#include <iostream>
#include <stdlib.h>

#include "sys/Log.h"

using namespace std;

int main(int argc, char* argv[])
{
    if (argc <= 1)
    {
        cout << "Please at lease specify addr to listen to" << endl;
        return 0;
    }

    const char* addr = argv[1];

    int port = 80;

    if (argc >= 3) port = atoi(argv[2]);

    if (argc >= 4) SetLogLevel(atoi(argv[3]));

    HttpServer server(addr, port);

    server.StartServer();

    char c;
    cin >> c;
    return 0;
}

