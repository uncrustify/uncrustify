#import <Foundation/Foundation.h>

extern "C" void function1(void *self, uint32_t *arg2, uint32_t * arg3);

MACRO1(void, function2, type1 arg1, type2 arg2, const type1 * arg3);

MACRO2(status_t, function3,
    void *arg1,
    const sp<IFoo>&arg2) {
}

MACRO2(type4, function4, const void **arg1, type1 arg2, const type3 * arg3, type4 arg4, type4 arg5, bool arg6) {
}
