template <bool a>
struct T {
        typedef int result;
};
template <bool a, bool b>
struct X {
        typedef typename T<a || b>::result result;
};

