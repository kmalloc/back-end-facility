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
            return new int;
        }
};


int * proc(double vv)
{
    cout << "normal proc!(" << vv << ")" << endl;
    return new int;
}

int main()
{
    dummy dum;
    function<int*, double> func = bind(&dummy::proc, &dum);
    int* i = func(23.23);

    delete i;

    function<int*, double> func2 = (&proc);
    i = func2(32.33);
    delete i;

    return 0;
}

