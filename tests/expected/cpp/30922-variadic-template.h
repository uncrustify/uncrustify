template<int __i, int... _Indexes, typename _IdxHolder, typename ...
         _Elements>
struct __index_holder_impl<__i, __index_holder<_Indexes...>,
                           _IdxHolder, _Elements ...>
{
	typedef typename __index_holder_impl<__i + 1,
	                                     __index_holder<_Indexes...,
	                                                    __i>,
	                                     _Elements ...>::type type;
};
