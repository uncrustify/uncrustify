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

void test(void)
{
return x != 0
&& x >= 1
&& x < 2
&& y >= 3
&& y < 4;
}
