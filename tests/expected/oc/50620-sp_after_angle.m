#import <Foundation/Foundation.h>

@interface TestClass2 : TestClassNavigationViewController {
    BOOL foo;
}

@implementation TestClass2

@end

@interface TestClass : TestClassNavigationViewController<UIWebViewDelegate, UIActionSheetDelegate> {
    NSURL *webpageUrl;
    UIWebView *webView;
    BOOL toolbarVisible;
    BOOL loading;
    NSString *endPrefix;
}

@implementation TestClass

@end
