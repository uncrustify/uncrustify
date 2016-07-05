@protocol spacingProtocol

@property NSInteger spacing;

@end

@interface cmt_insert

+ (cmt_insert*) shareInstance;

@property (readonly) BOOL isAvailable;

@property (copy) NSArray<NSString*>* contents;

- (void) updateContents:(NSArray<NSString*>*) inContents andRefresh:(BOOL) inRefresh;

@end

@implementation cmt_insert

+ (cmt_insert*) sharedInstance {
return nil;
}

- (BOOL) isAvailable {
return YES;
}

- (NSArray<NSString*>*) contents {
return @[];
}

- (void) setContents:(NSArray<NSString*>*) inContents {
}

- (void) updateContents:(NSArray<NSString*>*) inContents andRefresh:(BOOL) inRefresh {
}

@end

@interface cmt_insert_with_protocol (spacingProtocol)
@end

@implementation cmt_insert_with_protocol (spacingProtocol)

- (NSInteger) spacing {
return 0;
}

- (void) setSpacing:(NSInteger) inSpacing {
}

@end
