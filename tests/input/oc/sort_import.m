// should be ddd, eee, fff
#import "ddd"
#import "fff"
#import "sort_import.h"
#import "eee"

// should be aaa, ccc
#import "ccc"
#import "sort_import+internal.h"
#import "aaa"
// should be just bbb
#import "bbb"

#import "sort_import+public.h"
