#include <iostream>

using namespace std;

int main()
{
    int i = 0;

    while (i >= 0)
    {
        i++;
    }

    cout << "sizeof(short):" << sizeof(short) << endl;

    cout << "i:" << i << endl;
    cout << hex << i << endl;

    cout << "modulus:" << hex << i%100 << ", dec:" << dec << i%100 << endl;

    i = i + 1;

    cout << "i:" << dec << i << endl;
    cout << hex << i << endl;
}
