#import <Foundation/Foundation.h>

#define MACRO(foo) \
    if (_##foo == NULL) { \
        Log("do %s", str(foo)); \
        _##foo = (foo##_t *) process(PR_FLAG, str(foo)); \
    }

#define OVERLOAD(base, foo) \
    foo##_override_t *foo##_bar = (foo##_override_t *) process(base##_bar, str(foo)); \
    _##foo##_override = (foo##_override_t *) process(base##_cache, str(foo));
