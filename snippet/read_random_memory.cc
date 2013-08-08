#include <iostream>

using namespace std;

struct cs
{
    int a;
    int b;
};

void read()
{
    char*  add = (char*)0x00000fff;
    size_t i   = 1;

    while (i < ~0)
    {
        cout << "a:" << hex << add + i << ", v:" << ((cs*)(add + i))->a << endl;
        ++i;
    }
}

int main()
{
    read();
    cout << &((cs*)0)->b << endl;
    char* buf = new char[5];

    buf[0] = 1;
    buf[1] = 2;
    buf[2] = 3;
    buf[3] = 4;
    buf[4] = 0;

    cout << "free buf:" << hex << *(int*)buf << endl;
    delete [] buf;
    cout << "free buf:" << hex << *(int*)buf << endl;

    return 0;
}

