void (^(^complexBlock)(void (^)(void)))(void) = ^(void (^aBlock)(void)) {
  NSLog(@"Good");
  return ^{
    NSLog(@"Nice");
  };
};
