#import <Foundation/NSObject.h>

@interface Fraction : NSObject {
    int numerator;
    int denominator;
}

- (void)print;
- (void)setNumerator:(int)d;
- (void)setDenominator:(int)d;
- (int)numerator;
- (int)denominator;
- (void)setNumDen:(int)n:(int)d;
@end
