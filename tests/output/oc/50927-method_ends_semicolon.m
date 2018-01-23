#import <Foundation/Foundation.h>

@interface TestClass

+ (void)cancelRequest:(id)request;

@end

@implementation TestClass

// Occasionally there will be user errors where someone will
// copy the interface method declaration to implementation
// and leaves the semicolon
+ (void)cancelRequest:(id)request; {
}

@end
