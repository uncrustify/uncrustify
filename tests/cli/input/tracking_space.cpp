{
   a =     b +              111-55;
}
template<typename...A, int...B>
struct foo1:foo1<A..., (sizeof...(A)+B)...>
{
        foo1() {
                int x = sizeof...(A);
                bool b = x > 1;
        }
};
