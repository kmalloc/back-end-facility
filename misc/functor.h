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
            virtual function_base<ret_type, arg_type>* clone() const = 0;
    };

    template<class ret_type, class arg_type>
    class function_impl_normal: public function_base<ret_type, arg_type>
    {
        public:
            typedef ret_type (* NORM_PROC) (arg_type);
            explicit function_impl_normal(NORM_PROC proc): norm_proc_(proc) {}
            ret_type operator() (arg_type arg) { return norm_proc_(arg); }

            virtual function_impl_normal<ret_type, arg_type>* clone() const
            {
                return new function_impl_normal<ret_type, arg_type>(norm_proc_);
            }

        private:
            NORM_PROC norm_proc_;
    };

    template<class CS, class ret_type, class arg_type>
    class function_impl_class: public function_base<ret_type, arg_type>
    {
        public:
            typedef ret_type (CS::* PROC)(arg_type);

            function_impl_class(CS* obj, PROC proc): obj_(obj), proc_(proc) {}

            ret_type operator() (arg_type arg) { return (obj_->*proc_)(arg); }

            virtual function_impl_class<CS, ret_type, arg_type>* clone() const
            {
                return new function_impl_class<CS, ret_type, arg_type>(obj_, proc_);
            }

        private:
            CS* obj_;
            PROC proc_;
    };

    template<class CS, class ret_type, class arg_type>
    class function_impl_functor: public function_impl_class<CS, ret_type, arg_type>
    {
        public:
            typedef function_impl_class<CS, ret_type, arg_type> base;

            // TODO, check if CS define the corresponding function.
            explicit function_impl_functor(CS obj)
                : obj_(obj), base(&obj_, static_cast<ret_type(CS::*)(arg_type)>(&CS::operator()))
            {}

            ret_type operator()(arg_type arg) { return obj_(arg); }

            virtual function_impl_functor* clone() const
            {
                return new function_impl_functor<CS, ret_type, arg_type>(obj_);
            }

        private:
            CS obj_;
    };

    template<class CS, class ret_type, class arg_type>
    class function1
    {
        public:
            // TODO, check if CS define the corresponding function.
            function1(CS obj, arg_type arg)
                : obj_(obj), arg_(arg)
            {}

            ret_type operator()() { return obj_(arg_); }

        private:
            CS obj_;
            arg_type arg_;
    };

    template<class ret_type, class arg_type>
    class function
    {
        public:
            typedef ret_type (* NORM_PROC) (arg_type);

            explicit function(function_base<ret_type, arg_type>* fun)
                : fun_(fun)
            {}

            function(NORM_PROC proc = 0)
                : fun_(new function_impl_normal<ret_type, arg_type>(proc))
            {}

            template<class FUN>
            function(FUN obj)
                : fun_(new function_impl_functor<FUN, ret_type, arg_type>(obj))
            {}

            ret_type operator() (arg_type arg) { return fun_->operator()(arg); }

            ~function()
            {
                delete fun_;
            }

            function(const function& fun)
            {
                fun_ = fun.fun_->clone();
            }

            void operator=(const function& fun)
            {
                if (this == &fun) return;

                fun_ = fun.fun_->clone();
            }

        private:
            function_base<ret_type, arg_type>* fun_;
    };

    template<class CS, class ret_type, class arg_type>
    function<ret_type, arg_type> bind(ret_type (CS::* proc)(arg_type), CS* pc)
    {
        function_base<ret_type, arg_type>* pf = new function_impl_class<CS, ret_type, arg_type>(pc, proc);
        return function<ret_type, arg_type>(pf);
    }

    template<class F, class arg_type>
    struct result_of
    {
        static F getFun();
        static arg_type getArg();

        // gcc's equivalent of decltype before c++11
        typedef typeof((getFun())(getArg())) type;
    };
} // end namespace misc

#endif
