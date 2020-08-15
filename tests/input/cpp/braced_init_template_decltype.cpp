#include <algorithm>
#include <type_traits>

template<typename Arg, typename ... Args, typename std::enable_if <!std::is_same<Arg, decltype (std::make_index_sequence<5> { })>::value, int>::type = 0>
void foo(Arg &&arg, Args && ... args)
{

}
