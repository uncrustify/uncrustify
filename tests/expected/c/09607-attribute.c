__attribute__((visibility("default"))) int *i;
__attribute__((visibility("default"))) FooString *i;

#define DEFINE_NOTIFICATION(name) extern "C" __attribute__((visibility ("default"))) FooString *const name = #name;
