#define FOO(X) \
template <unsigned _blk_sz, typename _run_type, class __pos_type> \
inline X<_blk_sz, _run_type, __pos_type> operator - ( \
const X<_blk_sz, _run_type, __pos_type> & a, \
typename X<_blk_sz, _run_type, __pos_type>::_pos_type off) \
{ \
return X<_blk_sz, _run_type, __pos_type>(a.array, a.pos - off); \
} \
template <unsigned _blk_sz, typename _run_type, class __pos_type> \
inline X<_blk_sz, _run_type, __pos_type> & operator -= ( \
X < _blk_sz, _run_type, __pos_type > & a, \
typename X<_blk_sz, _run_type, __pos_type>::_pos_type off) \
{ \
a.pos -= off; \
return a; \
}

