#ifndef _H_NON_COPYABLE_H_
#define _H_NON_COPYABLE_H_

#define DECLARE_NONCOPYABLE(cs) \
    private:    \
        cs(const cs&);\
        const cs& operator=(const cs&);


class noncopyable
{
    protected:
        noncopyable(){};
        ~noncopyable(){};
    private:
        noncopyable(const noncopyable&);
        const noncopyable& operator=(const noncopyable&);
};

#endif

