/**
 * @file cmt_insert.m
 * Description
 *
 * $Id$
 */
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

/**
 * +[cmt_insert sharedInstance]
 *
 * @return TODO
 */
+ (cmt_insert*) sharedInstance {
	return nil;
}

/**
 * -[cmt_insert isAvailable]
 *
 * @return TODO
 */
- (BOOL) isAvailable {
	return YES;
}

/**
 * -[cmt_insert contents]
 *
 * @return TODO
 */
- (NSArray<NSString*>*) contents {
	return @[];
}

/**
 * -[cmt_insert setContents:]
 *
 * @param inContents TODO
 */
- (void) setContents:(NSArray<NSString*>*) inContents {
}

/**
 * -[cmt_insert updateContents:andRefresh:]
 *
 * @param inContents TODO
 * @param inRefresh TODO
 */
- (void) updateContents:(NSArray<NSString*>*) inContents andRefresh:(BOOL) inRefresh {
}

@end

@interface cmt_insert_with_protocol (spacingProtocol)
@end

@implementation cmt_insert_with_protocol (spacingProtocol)

/**
 * -[cmt_insert_with_protocol(spacingProtocol) spacing]
 *
 * @return TODO
 */
- (NSInteger) spacing {
	return 0;
}

/**
 * -[cmt_insert_with_protocol(spacingProtocol) setSpacing:]
 *
 * @param inSpacing TODO
 */
- (void) setSpacing:(NSInteger) inSpacing {
}

@end
