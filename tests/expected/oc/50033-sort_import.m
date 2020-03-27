// should be ddd, eee, fff
#import "ddd"
#import "eee"
#import "fff"

#import "aaa"
#import "ccc"
// should be just bbb
#import "bbb"

#import "sort_import+internal.h"
#import "sort_import+public.h"
#import "sort_import.h"

#import "Action+Internal.h"
#import "Action+Public.h"
#import "Action.h"
#import <UIKit/UIKit.h>

#include "Test+Internal.h"
#include "Test.h"

#import "Something.h"
#import "Something_Internal.h"
#import "Something_Public.h"

#import "AAA"
#include "DDD"
#import <KKK>
#import "BBB.h"
#include "CCC.h"
#import "EEE.h"
