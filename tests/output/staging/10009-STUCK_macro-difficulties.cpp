#define inline_2 __forceinline
#define inline(i) inline_##i
inline(2) f()
{
}
