#include <type_traits>

template<typename U, typename V, typename = std::enable_if_t<!std::is_convertible<U, V>::value && !std::is_same<U, V>::value>>
void foo(U &&u, V &&v)
{

}
