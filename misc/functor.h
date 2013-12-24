#ifndef __MISC_FUNCTOR__
#define __MISC_FUNCTOR__

template<class ret_type, class arg_type>
class function_base
{
    public:
        typedef ret_type (* NORM_PROC) (arg_type);

        function_base(NORM_PROC proc = 0): norm_proc_(proc) {}

        virtual ret_type operator() (arg_type arg)
        {
            if (norm_proc_) return norm_proc_(arg);

            return ret_type();
        }
    private:
        NORM_PROC norm_proc_;
};

template<class ret_type, class arg_type>
class function
{
    public:
        typedef ret_type (* NORM_PROC) (arg_type);

        function(function_base<ret_type, arg_type>* fun): fun_(fun), ref_(new int(1)) {}

        function(NORM_PROC proc = 0): fun_(new function_base<ret_type, arg_type>(proc)), ref_(new int(1)) {}

        ret_type operator() (arg_type arg) { fun_->operator()(arg); }

        ~function()
        {
            Release();
        }

        void Release()
        {
            *ref_ -= 1;
            if (*ref_ == 0)
            {
                delete ref_;
                delete fun_;
            }
        }

        function(const function& fun)
        {
            fun_ = fun.fun_;
            ref_ = fun.ref_;
            *ref_ += 1;
        }

        void operator=(const function& fun)
        {
            Release();
            fun_ = fun.fun_;
            ref_ = fun.ref_;
            *ref_ += 1;
        }

    private:

        int* ref_;
        function_base<ret_type, arg_type>* fun_;
};

template<class CS, class ret_type, class arg_type>
class function_impl: public function_base<ret_type, arg_type>
{
    public:

        function_impl(CS obj): obj_(obj) {}

        operator function_base<ret_type, arg_type>*() const {  return new function_impl<CS, ret_type, arg_type>(obj_); }

        ret_type operator() (arg_type arg) { return obj_(arg); }

    private:
        CS obj_;
};

template<class ret_type, class arg_type>
class function_impl_normal: public function_base<ret_type, arg_type>
{
    public:
        typedef ret_type (*PROC)(arg_type);

        function_impl_normal(PROC proc): proc_(proc) {}

        ret_type operator() (arg_type arg) { return *proc_(arg); }

        operator function_base<ret_type, arg_type>*() const { return new function_impl_normal<ret_type, arg_type>(proc_); }
    private:
        PROC proc_;
};

template<class CS, class ret_type, class arg_type>
class FuncHolder
{
    public:
        typedef ret_type (CS::* PROC)(arg_type);

        FuncHolder(CS* pc, PROC proc)
            :pc_(pc), proc_(proc)
        {
        }

        ret_type operator() (arg_type arg) {  return (pc_->*proc_)(arg); }
    private:
        CS* pc_;
        PROC proc_;
};

template<class CS, class ret_type, class arg_type>
function<ret_type, arg_type> bind(ret_type (CS::* proc)(arg_type), CS* pc)
{
    FuncHolder<CS, ret_type, arg_type> func_holder(pc, proc);
    return new function_impl<FuncHolder<CS, ret_type, arg_type>, ret_type, arg_type>(func_holder);
}

#endif

