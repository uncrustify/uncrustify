- (void)func {
  func2(^{
    switch (type) {
      case one:
        break;
      default:
        break;
    }
  });
}

- (void)func {
  func2(^{
    if(YES) {
      switch (type) {
        case one: break;
      }
    }
  });
}
