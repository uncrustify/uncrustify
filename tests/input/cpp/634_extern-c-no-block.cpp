extern "C" int* i;
extern "C" { int* i; }
int* i;
extern "C" NSString* i;
extern "C" { NSString* i; }
NSString* i;

__attribute__((visibility ("default"))) int* i;
__attribute__((visibility ("default"))) NSString* i;
