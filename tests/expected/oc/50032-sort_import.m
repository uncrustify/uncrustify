// should be ddd, eee, fff
#import "ddd"
#import "eee"
#import "fff"

#import "aaa"
#import "ccc"
// should be just bbb
#import "bbb"

#import "sort_import.h"
#import "sort_import+internal.h"
#import "sort_import+public.h"

#import "Action.h"
#import "Action+Internal.h"
#import "Action+Public.h"
#import <UIKit/UIKit.h>

#include "Test.h"
#include "Test+Internal.h"

#import "Something.h"
#import "Something_Internal.h"
#import "Something_Public.h"

#import "AAA"
#import "BBB.h"
#include "CCC.h"
#include "DDD"
#import "EEE.h"
#import <KKK>
