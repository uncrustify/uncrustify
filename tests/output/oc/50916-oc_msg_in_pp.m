#import <Foundation/Foundation.h>

#define UIColorFromRGB(rgbValue) [UIColor colorWithRed:((float) ((rgbValue & 0xFF0000) >> 16)) / 255.0 green:((float) ((rgbValue & 0xFF00) >> 8)) / 255.0 blue:((float) (rgbValue & 0xFF)) / 255.0 alpha:1.0]

#if TARGET_OS_IPHONE
#define GL_CONTEXT_ALLOC(parentContext) [[EAGLContext alloc] initWithAPI:GL_CONTEXT_VERSION(parentContext) sharegroup:[parentContext sharegroup]]
#else
#define GL_CONTEXT_ALLOC(parentContext) [[NSOpenGLContext alloc] initWithFormat:[[NSOpenGLPixelFormat alloc] initWithCGLPixelFormatObj:CGLGetPixelFormat([parentContext CGLContextObj])] shareContext:parentContext]
#endif

#define NSLocalizedString(key, comment) \
    [TestClassBundle localizedStringForKey:(key) value:@"" table:nil]

@interface TestClass : NSObject
@end

@implementation TestClass
@end
