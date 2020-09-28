#import "Entropy.h"

@implementation Entropy

- (void)deactivateEntropy {
    if (entropy 
        && (entropy->value() == 2) 
        || (entropy->value() == 4)) {
        [self callHome:(entropy->value()+6)];
    }
}

@end