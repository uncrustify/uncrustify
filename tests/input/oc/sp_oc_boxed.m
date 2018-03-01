
- (void) foo {
   NSArray* a = @[];
   NSDictionary* b = @{};
   NSArray<NSString*>* array = @[@"hello", @"world"];
   NSDictionary<NSString*, NSString*>* dictionary = @{@"foo":@"bar", @"foo2":@"bar2"};
   
   	NSString* const type = [pasteboard availableTypeFromArray:@[NSPasteboardTypeString]];
}
