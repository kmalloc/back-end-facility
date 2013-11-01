#include <iostream>
#include <map>
using namespace std;

class base
{
    public:

        int x;

    protected:

        int y;

    private:

        int z;
};

class deride: public base
{
    public:

        deride() { x = 3; y = 3; }
};

class deride2: public deride
{
    public:

        deride2() { y = 3; }
};

int main()
{
    std::map<int, int> m;
    cout << "map:" << m[12] << endl;

    deride d;
    deride2 d2;

    // d.x = 3;

    return 0;
}

