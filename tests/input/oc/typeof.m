#import <Foundation/Foundation.m>

@implementation MyViewController

- (void)method1 {
    __weak __typeof(self)weakSelf1 = self;
    __weak typeof(self)weakSelf2 = self;
    __weak MyViewController *weakSelf3 = self;
    NSString* srcStr = [[NSString alloc] initWithBytes: kShaderSource length: sizeof(kShaderSource) encoding: NSASCIIStringEncoding];
}

@end
