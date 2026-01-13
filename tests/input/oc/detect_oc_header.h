// Test file for detecting Objective-C content in .h headers
// This file should be auto-detected as OC without a language override

#import <Foundation/Foundation.h>

@interface MyClass : NSObject

@property (nonatomic, strong) NSString *name;
@property (nonatomic, assign) NSInteger count;

- (void)doSomething;
- (NSString *)getName;

@end
