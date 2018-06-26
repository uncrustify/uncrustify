#if __has_include(<tr1/unordered_set>)
#include <tr1/unordered_set>
#endif
#if __has_include("unordered_set.h") || __has_include_next(<tr1/unordered_set>)
#include <tr1/unordered_set>
#endif
