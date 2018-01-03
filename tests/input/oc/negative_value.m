#import <Foundation/Foundation.h>

int main(void) {
    int i = -1;

    NSNumber *foo1 = @-1;
    NSNumber *foo2 = @(-1);
    NSNumber *foo3 = @1;
    NSNumber *foo4 = @(1);
    NSNumber *foo5 = @(i);

    return 0;
}
