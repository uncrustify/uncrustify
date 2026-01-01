// Test case: Nested ternary with ObjC message calls containing ternaries
// This is a regression test for issue where a ternary expression that contains
// nested ObjC message calls (which themselves contain ternaries) causes the
// FOLLOWING OC message parameter colon to be incorrectly classified as CT_COND_COLON.
//
// The pattern is:
//   paramA:outerTernary ? [InnerMsg sel:innerTernary ? val : nil] : [OtherMsg sel:innerTernary ? val : nil]
//   paramB:simpleValue   <-- This colon should NOT get spaces added
//
// This differs from existing tests because the ternary branches contain ObjC message
// calls that themselves have ternary arguments, creating a complex nesting scenario.

@implementation TestClass

// Case 1: Basic nested ternary with inner OC message containing ternary
- (void)testBasicCase {
    _result =
    [[Builder alloc]
     paramA:condA
     paramB:useBig ? [Helper bigWithMin:useMin ? @(100) : nil] : [Helper smallWithMin:useMin ? @(50) : nil]
     paramC:simpleValue];
}

// Case 2: Multiple levels of nesting
- (void)testDeepNesting {
    _result =
    [[Builder alloc]
     first:value1
     second:condA ? [Helper methodA:condB ? [Inner get:condC ? @1 : nil] : nil] : [Helper methodB:nil]
     third:value3
     fourth:value4];
}

// Case 3: Real-world pattern with nested ternary in OC message
- (void)testRealWorldPattern {
    _viewModel =
    [[ViewModel alloc]
     initWithValue:props.value
     displayFlag:isEnabled
     thumbnailSize:useBigPreview ? [Helper biggerSizeWithMinWidth:shouldUseMin ? @(kMinWidth) : nil] : [Helper smallerSizeWithMinWidth:shouldUseMin ? @(kMinWidth) : nil]
     blurredViewModel:blurredViewModel];
}

// Case 4: Consecutive parameters with nested ternaries
- (void)testConsecutiveNestedTernaries {
    [object
     setA:condA ? [Helper a:innerA ? @1 : nil] : nil
     setB:condB ? [Helper b:innerB ? @2 : nil] : nil
     setC:simpleC
     setD:simpleD];
}

@end
