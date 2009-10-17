// should be ddd, eee, fff
using b.ddd;
using b.fff;
using b.eee;

// should be aaa, ccc
using ccc;
using aaa;
// should be just bbb
using bbb;

// should not change these, as it can't handle multi-line imports
using mango.ccc;
using mango.bbb,
      mango.aaa;

void foo();

