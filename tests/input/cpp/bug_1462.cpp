#include <type_traits>

template <
    typename... Args,
    typename E = typename std::enable_if<(sizeof...(Args) >= 1), bool>::type
>
void fun1(Args&& ...args)
{
}

template <
    typename... Args,
    typename E = typename std::enable_if<(sizeof...(Args) > 1), bool>::type
>
void fun2(Args&& ...args)
{
}

template <
    typename... Args,
    typename E = typename std::enable_if<(sizeof...(Args) < 3), bool>::type
>
void fun3(Args&& ...args)
{
}

