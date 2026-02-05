// Test 51012: Dictionary colon misclassified as ternary colon in OC message context
// When a ternary has an OC dictionary literal @{...} in the true branch,
// the colon inside the dictionary is incorrectly classified as the ternary colon,
// causing the real ternary colon to be classified as OC_COLON (no spacing)

@implementation Test

// Case 1: Dictionary with trailing comma in OC message - space before outer : removed
- (void)testCase1 {
    id result = [obj method:handler != nil ? @{kKey : handler, } : @{}];
}

// Case 2: Dictionary without trailing comma - works correctly
- (void)testCase2 {
    id result = [obj method:handler != nil ? @{kKey : handler} : @{}];
}

// Case 3: Multiple key-value pairs with trailing comma
- (void)testCase3 {
    id result = [obj method:flag ? @{@"a" : @1, @"b" : @2, } : @{}];
}

// Case 4: Nested dictionaries
- (void)testCase4 {
    id result = [obj method:flag ? @{@"outer" : @{@"inner" : @1}, } : @{}];
}

// Case 5: Real world pattern from FBInstreamAdCompactOverlayComponentSpec.mm
- (void)testCase5 {
    id result = [obj contextWithOptionalDependencies:props.actionHandler != nil ? @{kOptionalDependencyKey : props.actionHandler, } : @{}];
}

@end
