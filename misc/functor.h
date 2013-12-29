#ifndef __MISC_FUNCTOR__
#define __MISC_FUNCTOR__

namespace misc
{
    template<class ret_type, class arg_type>
    class function_base
    {
        public:
            function_base() {}
            virtual ~function_base(){}

            virtual ret_type operator() (arg_type) = 0;
    };

    template<class ret_type, class arg_type>
    class function_impl_normal: public function_base<ret_type, arg_type>
    {
        public:
            typedef ret_type (* NORM_PROC) (arg_type);
            function_impl_normal(NORM_PROC proc): norm_proc_(proc) {}
            ret_type operator() (arg_type arg) { return norm_proc_(arg); }
        private:
            NORM_PROC norm_proc_;
    };

    template<class ret_type, class arg_type>
    class function
    {
        public:
            typedef ret_type (* NORM_PROC) (arg_type);

            function(function_base<ret_type, arg_type>* fun): fun_(fun), ref_(new int(1)) {}

            function(NORM_PROC proc = 0): fun_(new function_impl_normal<ret_type, arg_type>(proc)), ref_(new in                                      t(1)) {}

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
            typedef ret_type (CS::* PROC)(arg_type);

            function_impl(CS* obj, PROC proc): obj_(obj), proc_(proc) {}

            ret_type operator() (arg_type arg) { return (obj_->*proc_)(arg); }

        private:
            CS* obj_;
            PROC proc_;
    };

    template<class CS, class ret_type, class arg_type>
    function<ret_type, arg_type> bind(ret_type (CS::* proc)(arg_type), CS* pc)
    {
        function_base<ret_type, arg_type>* pf = new function_impl<CS, ret_type, arg_type>(pc, proc);
        return function<ret_type, arg_type>(pf);
    }

} // end namespace misc

#endif
