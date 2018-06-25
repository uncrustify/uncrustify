#import <Foundation/Foundation.h>

#if TARGET_RT_BIG_ENDIAN
#define FourCC2Str(fourcc) (const char[]) { *((char *) &fourcc), *(((char *) &fourcc) + 1), *(((char *) &fourcc) + 2), *(((char *) &fourcc) + 3), 0}
#else
#define FourCC2Str(fourcc) (const char[]) { *(((char *) &fourcc) + 3), *(((char *) &fourcc) + 2), *(((char *) &fourcc) + 1), *(((char *) &fourcc) + 0), 0}
#endif

#if 1
#define SYNCHRONIZED_BEGIN(x) @synchronized(x) {
#define SYNCHRONIZED_END }
#else
#define SYNCHRONIZED_BEGIN(x)
#define SYNCHRONIZED_END
#endif

#define AUTORELEASEPOOL_BEGIN @autoreleasepool {
#define AUTORELEASEPOOL_END }
