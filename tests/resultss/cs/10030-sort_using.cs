// should be ddd, eee, fff
using b.ddd;
using b.eee;
using b.fff;

// should be aaa, ccc
using aaa;
using ccc;
// should be just bbb
using bbb;

// should not change these, as it can't handle multi-line imports
using mango.ccc;
using mango.bbb,
      mango.aaa;

void foo();

