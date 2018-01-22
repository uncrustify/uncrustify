struct
{
	int foo;
} bar;

extern "C"    int* i;
extern "C" {  int* i; }
int *i;
extern "C"    NSString* i;
extern "C" {  NSString* i; }
NSString* i;

__attribute__((visibility("default")))    int* i;
__attribute__((visibility("default")))    NSString* i;

#define DEFINE_NOTIFICATION(name) extern "C" __attribute__((visibility ("default")))    NSString*  const name = @#name;
