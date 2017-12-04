#include <utility>

template <typename Fun, typename ... Args>
inline decltype(auto)Invoke(Fun&& f, Args&&... args)
noexcept (noexcept (std::forward<Fun>(f)(std::forward<Args>(args) ...)))
{ return std::forward<Fun>(f)(std::forward<Args>(args) ...); }
