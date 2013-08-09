#include <iostream>
#include <vector>

using namespace std;

struct cs
{
    int a;
    int b;
};

template <class T, class D>
class RefValue
{
    public:
        RefValue(D ref): m_ref(ref)
        {
        }

        RefValue(const RefValue& rhs): m_ref(rhs.m_ref)
        {
        }

        operator T& () const
        {
            return m_ref;
        }

        RefValue& operator=(const RefValue& ref)
        {
            m_ref = ref.m_ref;
        }

    private:
        RefValue();

        T& m_ref;
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

class Base
{
    public:

        virtual void proc()
        {
            cout << "base proc" << endl;
        }
};


class RefCs: public Base
{
    public:

        RefCs(): m_id(m_c++)
        {
            cout << "RefCs ctor:" << m_id << endl;
        }

        RefCs(const RefCs& cs):m_id(m_c++)
        {
            cout << "copying, origin:" << cs.m_id << endl;
        }

        ~RefCs()
        {
            cout << "RefCs dtor:" << m_id << endl;
            m_id = -1;
        }

        virtual void proc()
        {
            cout << "deride proc:" << m_id << endl;
        }


    private:
        
        int m_id;
        static int m_c;
};

int RefCs::m_c = 0;

static vector<RefValue<Base, RefCs> > vd;

template<class T>
void TestClassAsCs(T& item)
{
    vd.push_back(RefValue<Base, RefCs>(item)); 
}

void PrintCs()
{
    for (size_t i = 0; i <  vd.size(); ++i)
    {
        Base& ref = vd[i];
        ref.proc();
    }
}

RefCs TempRefCs()
{
    return RefCs();
}

int main2()
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

int main()
{
    RefCs tmp = TempRefCs();

    cout << "starting..." << endl;

    {
        RefCs dr1, dr2;
        TestClassAsCs(dr1);
        TestClassAsCs(dr2);
    }

    PrintCs();

    cout << "clear vector" << endl;

    vd.clear();

    return 0;
}

