#include "functor.h"
#include <iostream>

using namespace std;
using namespace misc;

class dummy
{
    public:
        int* proc(double dv)
        {
            cout << "dummy proc!(" << dv << ")" << endl;
            return reinterpret_cast<int*>(555);
        }

        int operator()(std::string s) { cout << "in functor:" << s << endl; return 233; }
};

int proc(double vv)
{
    cout << "normal proc!(" << vv << ")" << endl;
    return 234;
}

int main()
{
    dummy dum;
    function<int*, double> func = bind(&dummy::proc, &dum);
    void* i = func(23.23);

    cout << "res of func:" << i << ", expected:" << hex << 555 << endl;

    function<int, double> func2 = (&proc);
    int i2 = func2(32.33);
    cout << "res of func2:" << i2 << ", expected:" << hex << 234 << endl;

    int k = 0;
    function<int, std::string> f3 = dum;
    k = f3("wwww");

    cout << "res of f3:" << k << ", expected:" << 233 << endl;
    return 0;
}

