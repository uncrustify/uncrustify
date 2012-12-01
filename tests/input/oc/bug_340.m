#import <Cocoa/Cocoa.h>

@implementation MyDocument
- (void) locationManager: (CLLocationManager*) manager didFailWithError:(NSError *)error {

  [UIAlertView showError: error
               withTitle: NSLocalizedString(@"Your location cannot be determined",
                               @"The location of the user cannot be guessed")
                 message: NSLocalizedString(@"You can try again by pressing the refresh button",
                             @"Recovery suggestion when the location cannot be found")];
}

@end
