#import <Foundation/Foundation.h>

@interface TestClass : NSObject
@end

@implementation TestClass

- (void) method1 {
    [session mergeCommonMovieItems:^(NSURL *exportURL, NSError *exportError) {
        NSDictionary *settings = [self getSettings];

        [session postSessionWithCallback:^(NSError *error, id d) {
            if (error == nil) {
                [session uploadSessionWithCallback:^(NSError *error, id d) {
                    NSLog(@"OK");
                }];
            } else {
                NSLog(@"Something went wrong: %@", error);
                return;
            }
        }];
    }];
}

- (void)postSelection:(NSString *)testName
            selection:(NSString *)selection {
    dispatch_async(dispatch_get_main_queue(), ^{
        [self warmup:^{
            [self setReady];
        }];
    });

    dispatch_after(retryTime, dispatch_get_main_queue(), ^(void) {
        [self postSelection:testName selection:selection];
    });

    [TestClassRequest performMethod:TestClassRequestMethodPOST
                    responseHandler:^(NSURLResponse *response) {
                        dispatch_after(retryTime, dispatch_get_main_queue(), ^(void) {
                            [self postSelection:testName selection:selection];
                        });
                    }];

    [UIView transitionWithView:self.view.window duration:0.75 options:UIViewAnimationOptionTransitionFlipFromRight animations:^{
        [self presentViewController:viewController animated:NO completion:nil];
    } complete:^{}];
}

- (void)closeEditor {
    dispatch_async(dispatch_get_main_queue(), ^{
        if ([[TestClass sharedInstance] TestClassController] != nil && [[[TestClass sharedInstance] TestClassController] isKindOfClass:[TestClassSocialViewController class]]) {
            [[TestClass sharedInstance].TestClassTransitionController transitionToViewController:[[TestClass sharedInstance] TestClassController] withCompletitionHandler:^{
                [[TestClass sharedInstance] setTestClassVideoPlayerViewController:nil];
            }];
        } else if ([[TestClass sharedInstance] TestClassController] != nil && [[[TestClass sharedInstance] TestClassController] isKindOfClass:[TestClassModalViewController class]]) {
            [[TestClass sharedInstance].TestClassTransitionController transitionToViewController:nil withCompletitionHandler:^{
                [[TestClass sharedInstance] setTestClassVideoPlayerViewController:nil];
            }];
        } else {
            [[TestClass sharedInstance] hideTestClass];
        }
    });
}

- (void)testMethodWrapper {
    SEL testMethodForSelectorSel = @selector(testMethod:);
    __block void *testMethodForSelectorBlock = TestFunction(encoderTest, testMethodForSelectorSel, ^id (__typeof (encoderTest) self, SEL aSelector) {
        NSLog(@"OK");
    });
}

- (void) method2 {
    [TestClassRequest performMethod:TestClassRequestMethodPOST
                    responseHandler:^(NSURLResponse *response, NSData *responseData, NSError *error) {
                        [self checkPermissions:connection withCallback:^(NSError *error, id data) {
                            [self bar];
                        }];
                    }];
}

- (void) method3 {
    [TestClassRequest performMethod:TestClassRequestMethodPOST responseHandler:^(NSURLResponse *response, NSData *responseData, NSError *error) {
        [self checkPermissions:connection withCallback:^(NSError *error, id data) {
            [self bar];
        }];
    }];
}

@end
