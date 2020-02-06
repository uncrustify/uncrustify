@implementation SomeClass
- (void)someMethod {
    enumerateItems(
        ^(NSInteger section) {
    });
}

- (void)someOtherMethod {
    items.enumerateItems(
        ^(NSInteger section, NSInteger index, id<NSObject> object, BOOL *stop) {
        enumerator(index, object, TypeInsert);
    },
        nil,
        some_param
        );
}

@end
