// should be ddd, eee, fff
#import "sort_import.h"
#import "ddd"
#import "eee"
#import "fff"

// should be aaa, ccc
#import "sort_import+internal.h"
#import "aaa"
#import "ccc"
// should be just bbb
#import "bbb"

#import "sort_import+public.h"
