#import <Foundation/Foundation.h>

@interface TestClass : NSObject
@end

@implementation TestClass

- (void)session_thumbnail_url:(NSDictionary *)data {
    [session mergeCommonMovieItems:^(NSURL *exportURL, NSError *error) {
#if 0
        [response setValue:[thumbnailUrl absoluteString] forKey:@"thumbnail_url"];
#else
        NSString *extension = [thumbnailUrl pathExtension];
        NSData *imageData = [NSData dataWithContentsOfURL:thumbnailUrl];
        NSString *base64EncodedImage = [TestClassCommon Base64Encode:imageData];

        NSString *base64Image = nil;
        if ([extension isEqualToString:@"jpg"] == YES) {
            base64Image = [NSString stringWithFormat:@"data:image/jpg;base64, %@", base64EncodedImage];
        } else {
            base64Image = [NSString stringWithFormat:@"data:image/png;base64, %@", base64EncodedImage];
        }
        [response setValue:base64Image forKey:@"thumbnail_url"];
#endif

        [TestClassWebViewController sendEvent:[NSString stringWithFormat:@"session_thumbnail:%@", sessionId] withArgs:response];
    }];
}

- (void)addFoo:(NSDictionary *)postData {
    [TestClassRequest performMethod:TestClassRequestMethodPOST
                         onResource:resource
                    usingParameters:postData
                        withAccount:[TestClass account]
             sendingProgressHandler:nil
                    responseHandler:^(NSURLResponse *response, NSData *responseData, NSError *error) {
                        NSHTTPURLResponse *httpResponse = (NSHTTPURLResponse *) response;
                        if ([httpResponse statusCode] == 200) {
#if DEBUG
                            NSString *rsp = [[NSString alloc] initWithData:responseData encoding:NSUTF8StringEncoding];
                            TestClassLog(@"TOGGLE CONNECTION ADDED response:%li responseData:%@ error:%@", (long) [((NSHTTPURLResponse *) response) statusCode], rsp, [error localizedDescription]);
#endif
                            NSJSONSerialization *jsonConnection = [responseData TestClassJSONObject];
                        }
                    }];
}

@end
