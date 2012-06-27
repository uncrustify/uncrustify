NSArray *test = @[];
NSNumber *test = @ (42 * 2);
NSNumber *test = @4.0;
NSDictionary *test = @{@"foo":@"bar"};

@implementation UrlTemplateTest
- (void)test {
       NSString *test = @"";
       NSString *string = [[NSMutableString alloc] initWithString:@""];
       STAssertEqualObjects(string, @"", nil);
}
@end
