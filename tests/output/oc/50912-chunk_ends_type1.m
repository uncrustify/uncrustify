#import <Foundation/Foundation.h>

@interface TestClass : NSObject
@end

@implementation TestClass

- (void)uploadWithClientData:(NSMutableDictionary *)data withCallback:(TestClassCallback)callback {
    TestClassSessionInstance *session = [[TestClassSession sharedInstance] currentOrLastSession];

    if (session == nil || data == nil) {
        if (callback != nil)
            return callback(nil, nil);
        return;
    }
    [session mergeCommonMovieItems:^(NSURL *exportURL, NSError *exportError) {
        if (exportError != nil)
            return callback(exportError, nil);
        NSDictionary *settings = [self getSettings];
    }];
}

@end
