#define STDTHREADCREATE_GOTO(var, func, RES, label, ...) \
        STDTHREADCREATE_IMPL(var, func, do { RES = Error; goto label; } while(0), __VA_ARGS__)

#define RUN_ONCE(f, body) f(do { body; } while(0))
