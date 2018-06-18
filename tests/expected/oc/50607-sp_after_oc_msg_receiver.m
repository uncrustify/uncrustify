#import <Foundation/Foundation.h>

@implementation TestClass

- (void)testMethod {
    NSData *jsonData = [self dataUsingEncoding:NSUTF8StringEncoding];
    id jsonParsedObj = [jsonSerializationClass JSONObjectWithData:jsonData options:0 error:&jsonError];
    NSString *ret = [[TestClass sharedInstance]testString];
}

@end
