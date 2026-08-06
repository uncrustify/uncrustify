template<typename T>
struct Traits;

template<typename C, typename R, typename ... Args>
struct Traits<R ( C::* )( Args... )>
{
};