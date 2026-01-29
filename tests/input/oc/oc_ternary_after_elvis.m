// Issue 51011: Space removed before outer ternary colon when true branch contains elvis (?:)
// Pattern: a ? b ?: c : d - the space before the final : is incorrectly removed

@implementation Test

// Case 1: Simple outer ternary with elvis in true branch
- (id)testCase1 {
    return condition ? value ?: fallback : defaultValue;
}

// Case 2: Real world example from FBVideoAvengersUFIComponentSpec.mm
- (NSString *)draftCommentKey:(BOOL)shouldDelegate pageType:(NSString *)pageType {
    return shouldDelegate ? pageType ?: kDefaultKey : kDefaultKey;
}

// Case 3: Real world example from HeraHostEventLoggerDeviceCacheManager.m
- (id)deviceInfoForId:(NSUUID *)deviceId {
    return deviceId ? _cache[deviceId] ?: _fallbackInfo : _fallbackInfo;
}

// Case 4: Multiple chained elvis before outer colon
- (id)testCase4 {
    return cond ? a ?: b ?: c : defaultVal;
}

// Case 5: Elvis in both branches
- (id)testCase5 {
    return cond ? a ?: b : c ?: d;
}

// Case 6: Nested in expression
- (void)testCase6 {
    NSString *result = [self process:flag ? val ?: backup : other];
}

@end
