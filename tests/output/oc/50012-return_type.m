#import <Foundation/NSObject.h>
#import <stdio.h>

@interface Fraction : NSObject
{
    int numerator;
    int denominator;
}
-(void)print;
-(void)setNumerator: (int) d;
-(void)setDenominator: (int) d;
-(int) numerator;
-(int) denominator;
-(void)setNumDen: (int) n: (int) d;
@end

@implementation Fraction
-(void)print
{
    printf("%i/%i", numerator, denominator);
}

-(void)setNumerator: (int) n
{
    numerator = n;
}

-(void)setDenominator: (int) d
{
    denominator = d;
}

-(int)denominator
{
    return(denominator);
}

-(int)numerator
{
    return(numerator);
}
@end
