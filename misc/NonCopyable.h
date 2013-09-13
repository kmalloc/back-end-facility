#ifndef _H_NON_COPYABLE_H_
#define _H_NON_COPYABLE_H_


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

