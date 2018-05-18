/**
 * @file cmt_insert2.m
 * Description
 *
 * $Id$
 */
NSURLResourceKey const NSURLCanonicalPathKey API_AVAILABLE(macosx(10.12), ios(10.0), watchos(3.0), tvos(10.0));

@interface foo ()

@property BOOL usesStrongWriteBarrier API_DEPRECATED("Garbage collection no longer supported", macosx(10.5, 10.12), ios(2.0, 10.0), watchos(2.0, 3.0), tvos(9.0, 10.0));

static const NSWindowStyleMask NSResizableWindowMask API_DEPRECATED_WITH_REPLACEMENT("NSWindowStyleMaskResizable", macosx(10.0, 10.12)) = NSWindowStyleMaskResizable;

- (NSString*) extensionForType:(NSString*) inFileType API_AVAILABLE(macosx(10.12), ios(10.0), watchos(3.0), tvos(10.0));

@end

@implementation foo

/**
 * -[foo filePromiseProvider:fileNameForType:]
 *
 * @param inFilePromiseProvider TODO
 * @param inFileType TODO
 * @return TODO
 */
- (NSString*) filePromiseProvider:(NSFilePromiseProvider*) inFilePromiseProvider fileNameForType:(NSString*) inFileType API_AVAILABLE(macos(10.12)) {
}
/**
 * -[foo filePromiseProvider:fileNameForType:]
 *
 * @param inFilePromiseProvider TODO
 * @param inFileType TODO
 * @return TODO
 */
- (NSString*) filePromiseProvider:(NSFilePromiseProvider*) inFilePromiseProvider fileNameForType:(NSString*) inFileType API_DEPRECATED_WITH_REPLACEMENT(macos(10.12))
{
}
/**
 * -[foo filePromiseProvider:fileNameForType:]
 *
 * @param inFilePromiseProvider TODO
 * @param inFileType TODO
 * @return TODO
 */
- (NSString*) filePromiseProvider:(NSFilePromiseProvider*) inFilePromiseProvider fileNameForType:(NSString*) inFileType API_UNAVAILABLE(macos(10.12)) {
}

@end
