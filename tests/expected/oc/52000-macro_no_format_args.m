// Test file for macro-no-format-args functionality
// This tests that Swift interop macros preserve their original formatting

#import <Foundation/Foundation.h>

// Test NS_SWIFT_NAME with getter: prefix - colon should NOT have space added after it
typedef struct {
    BOOL isManaged;
} DeviceInfoStruct NS_SWIFT_NAME(getter:DeviceInfoStruct.isManaged());

// Test NS_SWIFT_NAME with trailing colon in parameter name
typedef struct {
    BOOL isAvailable;
} OSBuildStruct NS_SWIFT_NAME(OSBuildStruct.isAvailable(majorVersion:));

// Test HELPER_SWIFT_RENAMED with type parameter
@interface SomeClass : NSObject
- (void)oldMethodName:(NSString *)param HELPER_SWIFT_RENAMED(newMethodName(parameter:));
@end

// Test HELPER_SWIFT_REDIRECT with type cast
extern void someFunction(int x) HELPER_SWIFT_REDIRECT(OtherClass.someFunction(_:));

// Test nested angle brackets and colons - should preserve formatting
typedef NS_ENUM(NSInteger, MyEnum) {
    MyEnumValue1 NS_SWIFT_NAME(value1),
    MyEnumValue2 NS_SWIFT_NAME(value2),
} NS_SWIFT_NAME(MySwiftEnum);

// Test complex macro with multiple colons
@interface TestClass : NSObject
- (instancetype)initWithFoo:(id)foo
    bar:(id)bar NS_SWIFT_NAME(init(foo:bar:));
@end

// Test that normal code still gets properly formatted
void normalFunction(int a, int b, int c) {
    int x = a + b + c;
    NSArray *arr = @[@1, @2, @3];
}

// Test HELPER_SWIFT_REFINED with two arguments (module name and Swift signature)
// These macros have two arguments separated by comma - both should be preserved
HELPER_SWIFT_REFINED(MyModule, MyStruct.string(forCtl:type:))
FOUNDATION_EXTERN NSString *__nullable MyAPIGetString(int32_t ctl, int32_t type, NSError *__nullable *__nullable error);

// Test HELPER_SWIFT_REFINED with nested parentheses and multiple colons inside @interface
@interface TestClassWithRefinedMethods : NSObject
{
    SOME_MACRO NSInteger _someIVar;
}
HELPER_SWIFT_REFINED(MyModule, MyClass.init(param1:param2:param3:))
- (instancetype)initWithParam1:(id)p1 param2:(id)p2 param3:(id)p3;
@end

// Test random macro in first line of interface
@interface TestClassWithRandomMethods : NSObject
    RANDOM_MACRO(something_random)
- (instancetype)initWithParam1:(id)p1 param2:(id)p2 param3:(id)p3;
@end

// Test random macro in first line of interface AFTER {}
@interface TestClassWithRandomMethods2 : NSObject
{
}
RANDOM_MACRO(something_random)
- (instancetype)initWithParam1:(id)p1 param2:(id)p2 param3:(id)p3;
@end

// Test that function prototypes following macro-no-format-args are classified correctly
// This tests the combine_fix_mark.cpp fix - functions should be FUNC_PROTO, not FUNC_CALL
HELPER_SWIFT_REFINED(MyModule, MyClass.getData())
FOUNDATION_EXTERN NSData *GetDataFunction(void);

HELPER_SWIFT_REFINED(MyModule, MyClass.getString(forKey:))
FOUNDATION_EXTERN NSString *GetStringFunction(NSString *key);

// Test multiple parameters - pointer spacing should be preserved
HELPER_SWIFT_REFINED(MyModule, MyClass.process(data:error:))
FOUNDATION_EXTERN BOOL ProcessDataFunction(NSData *data, NSError **error);

// Test NS_SWIFT_NAME followed by function prototype
NS_SWIFT_NAME(getter:MyClass.sharedInstance())
FOUNDATION_EXTERN id GetSharedInstance(void);

// Test that inline functions after macros are formatted correctly
NS_SWIFT_NAME(getter:MyClass.version())
NS_INLINE NSString *GetVersionString(void) {
    return @"1.0.0";
}
