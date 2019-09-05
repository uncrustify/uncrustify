// should be ddd, eee, fff
#include "ddd"
#include "fff"
#include "eee"

// should be aaa, ccc
#include "ccc"
#include "aaa"
// should be just bbb
#include "bbb"

// should be a, aa
#include "aa"
#include "a"

// should be a, aa
#include <aa>
#include <a>

// should be b, a
#include <a>
#include "b"
