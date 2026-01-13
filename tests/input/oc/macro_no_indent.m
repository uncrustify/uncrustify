// Test file for macro-no-indent functionality
// This tests that certain macros do not affect indentation
// Unlike macro-close (which expects matching open/close pairs),
// macro-no-indent is for single-line attribute macros

#import <Foundation/Foundation.h>

NS_ASSUME_NONNULL_BEGIN

// Test 1: Basic interface with NS_UNAVAILABLE - code after macro should not be indented
@interface TestClass : NSObject

- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithValue:(int)value NS_DESIGNATED_INITIALIZER;
- (void)doSomething;

@end

// Test 2: NS_SWIFT_NAME on enum values inside NS_ENUM
// This was the original bug - macro-close caused parse errors in enums
typedef NS_ENUM(NSInteger, TestEnum) {
    TestEnumValueOne NS_SWIFT_NAME(one),
    TestEnumValueTwo NS_SWIFT_NAME(two),
    TestEnumValueThree NS_SWIFT_NAME(three),
};

// Test 3: Multiple attribute macros on methods
@interface AnotherClass : NSObject

+ (instancetype)new NS_UNAVAILABLE;
- (instancetype)init NS_UNAVAILABLE;
- (instancetype)initWithFoo:(id)foo NS_DESIGNATED_INITIALIZER;

@end

// Test 4: Custom macro defined in config
void someFunction(void) CUSTOM_ATTRIBUTE_MACRO;

// Test 5: Interface with macros - properties and methods should align
@interface OuterClass : NSObject
{
}
INIT_AND_NEW_UNAVAILABLE;
@property (nonatomic) int value;
- (void)methodOne NS_UNAVAILABLE;
- (void)methodTwo;
@end

NS_ASSUME_NONNULL_END

// Test 6: Code after NS_ASSUME_NONNULL_END should be at correct indent level
@interface PostNullabilityClass : NSObject
- (void)normalMethod;
@end

// Test 7: RUNTIME_PROTOCOL followed by @protocol declaration
// This tests the combine.cpp fix - angle brackets should be protocol lists, not generics
RUNTIME_PROTOCOL
@protocol MyProtocol <NSObject>
- (void)requiredMethod;
@optional
- (void)optionalMethod;
@end

// Test 8: NON_RUNTIME_PROTOCOL followed by @protocol with conformance
NON_RUNTIME_PROTOCOL
@protocol AnotherProtocol <NSObject, NSCopying>
- (id)copyWithZone:(NSZone *)zone;
@end

// Test 9: @protocol with trailing comment - spacing should be preserved
RUNTIME_PROTOCOL
@protocol CommentedProtocol <NSObject> // This is a comment
- (void)someMethod;
@end

// Test 10: Function prototypes after macro-no-indent should be classified correctly
// This tests the combine_fix_mark.cpp fix
NS_ASSUME_NONNULL_BEGIN
FOUNDATION_EXTERN NSString *GetSomeString(void);
FOUNDATION_EXTERN NSData *GetSomeData(NSString *key);
NS_ASSUME_NONNULL_END
