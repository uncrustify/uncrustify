// should be ddd, eee, fff
#include "ddd"
#include "eee"
#include "fff"

// should be aaa, ccc
#include "aaa"
#include "ccc"
// should be just bbb
#include "bbb"

// should be a, aa
#include "a"
#include "aa"

// should be a, aa
#include <a>
#include <aa>

// should be b, a
#include "b"
#include <a>
