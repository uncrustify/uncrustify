#import <Foundation/Foundation.h>

#define TESTCLASS_SINGLETON_FOR_CLASS(classname, accessorname) \
    static classname *shared##classname = nil;

#define PUSH_CONTEXT(contextArg) \
    EAGLContext *oldContext = [EAGLContext currentContext]; \
    if (oldContext != contextArg) { \
        [EAGLContext setCurrentContext:contextArg]; \
    }

#define POP_CONTEXT(contextArg) \
    if (oldContext != contextArg) { \
        [EAGLContext setCurrentContext:oldContext]; \
    }

#define CURRENT_CONTEXT \
    NSOpenGLContext *currentContext = [NSOpenGLContext currentContext];

@interface TestClass : NSObject
@end

@implementation TestClass

TESTCLASS_SINGLETON_FOR_CLASS(TestClass, sharedInstance);

- (void) drawSomething {
	PUSH_CONTEXT(_context);
	POP_CONTEXT(_context);
}

@end
