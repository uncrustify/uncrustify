// Test file for macro-no-format-args edge cases
// Tests nested parentheses, multiple macros, and boundary conditions

#import <Foundation/Foundation.h>

// Test deeply nested parentheses inside macro - should preserve
void testNestedParens(void) NS_SWIFT_NAME(test(nested(deep(parens:))));

// Test macro with angle brackets
@interface GenericClass<ObjectType> : NSObject
- (void)processObject:(ObjectType)obj NS_SWIFT_NAME(process(object:));
@end

// Test macro at end of line with different content types
typedef void (^ CompletionHandler)(NSError *error) NS_SWIFT_NAME(CompletionHandler);

// Test custom macro defined in config
void customMacroTest(int x, int y) CUSTOM_MACRO(custom:args:here:);

// Test code before and after macro should be formatted
void beforeMacro(int a, int b) {
    int x = a + b;
}
void withMacro(int c, int d) NS_SWIFT_NAME(with(c:d:));
void afterMacro(int e, int f) {
    int y = e + f;
}

// Test empty macro args
void emptyArgs(void) NS_SWIFT_NAME();

// Test single argument
void singleArg(void) NS_SWIFT_NAME(renamed);

// Test underscore placeholder
void withUnderscore(int x) NS_SWIFT_NAME(method(_:));
