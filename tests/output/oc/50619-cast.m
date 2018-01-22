#import <Foundation/Foundation.h>

@interface TestClass : NSObject
@end

@implementation TestClass

- (void)foo {
    previewViewController.previewControllerDelegate = (id<TestClassDelegate>) [TestClass sharedInstance];
}

@end
