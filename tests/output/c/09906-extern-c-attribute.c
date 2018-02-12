struct {
    int foo;
} bar;

extern "C" int *i;
extern "C" {  int *i; }
int *i;
extern "C" FooString *i;
extern "C" {  FooString *i; }
FooString *i;

__attribute__((visibility("default"))) int *i;
__attribute__((visibility("default"))) FooString *i;

#define DEFINE_NOTIFICATION(name) extern "C" __attribute__((visibility("default"))) FooString *const name = #name;
