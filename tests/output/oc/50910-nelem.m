#import <Foundation/Foundation.h>

/* get #of elements in a static array */
#ifndef NELEM
#define NELEM(x) ((int) (sizeof(x) / sizeof((x)[0])))
#endif

int main(void) {
    return 0;
}
