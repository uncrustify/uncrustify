#define DEFINE_SYMBOL(ret, fn, ...)   ret (*OVERRIDE_SYMBOL(fn))(__VA_ARGS__)
#define DEFINE_SYMBOL(ret, fn, ...)   ret (*OVERRIDE_SYMBOL(OVERRIDE_SYMBOL2(fn)))(__VA_ARGS__)
#define DEFINE_SYMBOL(ret, fn, ...)   ret (*OVERRIDE_SYMBOL(fn)())(__VA_ARGS__)
