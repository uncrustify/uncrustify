// Test case: OC message with ternary inside outer ternary
// This is a regression test for issue where an OC message containing ternaries
// with the "value:nil" pattern (no space before colon) is inside an outer ternary.
// The subsequent OC selector colons are incorrectly marked as CT_COND_COLON.
//
// The pattern is:
//   auto result = outerCondition
//   ? [Obj
//      context:[[Inner alloc]
//               pageID:!isOwner ? actorID:nil       <- ternary with value:nil
//               profileID:isOwner ? actorID:nil     <- subsequent OC colon gets spaces (BUG)
//               tapAction:@selector(handle:)]       <- this also gets spaces (BUG)
//      data:nil]
//   : nil;

@implementation TestClass

// Case 1: OC message with ternaries inside outer ternary
- (void)testOuterTernaryWithInnerTernaries {
    auto const result =
    condition
    ? [Obj
       newWithComponent:comp
       context:[[Context alloc]
                initWithToolbox:toolbox
                postID:postID
                adID:adID
                pageID:!isOwner ? actorID:nil
                profileID:isOwner ? actorID:nil
                tapAction:@selector(handle:)
       ]
       data:nil]
    : nil;
}

// Case 2: Simpler case - just consecutive ternaries with value:nil pattern
- (void)testConsecutiveTernariesValueNil {
    [obj
     pageID:!isOwner ? actorID:nil
     profileID:isOwner ? actorID:nil
     tapAction:@selector(handle:)];
}

@end
