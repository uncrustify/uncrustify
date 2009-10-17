template <bool a>
struct T {
        typedef int result;
};
template <bool a, bool b>
struct X {
        typedef typename T<a || b>::result result;
};

template <class T>
class new_alloc {
public:
    void deallocate (int* p, int /*num*/)
    {
        T::operator delete((void *)p);
    }
};

