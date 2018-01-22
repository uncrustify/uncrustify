#import <Foundation/Foundation.h>

@implementation TestClass

- (NSDictionary *)constructUploadHeaders {
    NSDictionary *headers;

    headers = @{
        @"Content-Length":[@(contentLength)stringValue],
        @"Content-Length2": [contentLength stringValue],
        @"Content-Disposition": [NSString stringWithFormat:@"name=\"%@\"; filename=\"%@\"", @"asset_data", identifier],
        @"Content-Type": @"application/octet-stream",
        @"ETag": uploadEtag
    };

    return headers;
}

- (BOOL)isStrimmed {
    if ([(TestClassVideoEditorView *)   (self.parentComponentsView.superview)isTrimmed]) {
    	return YES;
    }
    if ([((TestClassVideoEditorView *)   self.parentComponentsView.superview)isTrimmed]) {
    	return YES;
    }
    return NO;
}

- (void)session {
    TestCaseSessionInstance *session = ((TestClassVideoEditorView *)  self.parentComponentsView.superview).session;

    [(TestClassVideoEditorView *)   (  self.parentComponentsView.superview) closeEditor];

    [menubutton.badge setBadgeText:[@(count + 1) stringValue]];
}

@end
