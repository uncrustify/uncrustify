#import "FOOAddressBookManager.h"
#import <React/RCTBridge.h>
#import <React/RCTEventDispatcher.h>

/*
 * #define RCT_EXPORT_METHOD(method) \
 *  - (void)__rct_export__##method { \
 *    __attribute__((used, section("__DATA,RCTExport"))) \
 *    static const char *__rct_export_entry__[] = { __func__, #method }; \
 *  } \
 *  - (void)method \
 */

@interface FOOAddressBook : NSObject
@end

@implementation FOOAddressBookManager

RCT_EXPORT_MODULE(FOOAddressBook)

RCT_EXPORT_METHOD(getAddresses:(NSDictionary *) data callback:(RCTResponseSenderBlock) callback)
{
    NSMutableArray *addresses = [[FOOAddressBook sharedInstance] getAddresses:data];
    if (addresses != nil)
        callback(@[[NSNull null], addresses]);
    else
        callback(@[@"getAddresses returned nil."]);
}

RCT_EXPORT_METHOD(getStatus:(RCTResponseSenderBlock) callback)
{
    callback(@[[NSNull null], [[FOOAddressBook sharedInstance] getStatus]]);
}

RCT_EXPORT_METHOD(requestAccess:(RCTResponseSenderBlock) callback)
{
    [[FOOAddressBook sharedInstance] requestAccess:^(NSString *status) {
        callback(@[[NSNull null], status]);
    }];
}

@end
