// Test file for macro-no-indent edge cases
// Tests scenarios with consecutive macros, macros with arguments,
// and interaction with other language constructs

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// Test 1: Consecutive macro-no-indent macros
@interface ConsecutiveMacros : NSObject
- (instancetype)init NS_UNAVAILABLE;
+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)initWithCoder:(NSCoder *)coder NS_UNAVAILABLE;
- (void)normalMethod;
@end

// Test 2: Macros at different nesting levels
@interface NestedTest : NSObject
@property (nonatomic, readonly) NSString *name;
- (void)outerMethod NS_UNAVAILABLE;
@end

@implementation NestedTest
- (void)implementationMethod {
    if (YES) {
        // Code inside blocks should still indent normally
        NSLog(@"test");
    }
}
@end

// Test 3: Macros in protocol declarations
@protocol TestProtocol <NSObject>
@required
- (void)requiredMethod;
@optional
- (void)optionalMethod NS_UNAVAILABLE;
@end

// Test 4: Struct with attribute macro
typedef struct {
    int field1;
    int field2;
} TestStruct NS_SWIFT_NAME(SwiftTestStruct);

// Test 5: Function declarations with macros
void functionOne(void) NS_UNAVAILABLE;
void functionTwo(int param) NS_SWIFT_NAME(function(with:));
void functionThree(void);

NS_ASSUME_NONNULL_END
