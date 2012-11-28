#import <Cocoa/Cocoa.h>

@implementation MyDocument

-(NSString *) appPath
{
   [AClass AFunc];
   return [NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) lastObject];
}

@end
