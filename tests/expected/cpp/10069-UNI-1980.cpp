// First: scan more FAKE_FUNCTION diffs and see how common this problem is.

// The & should be attached to RefType because it's in a function prototype. Most likely being detected as ARITH.

// We need to figure out how to support this with some setting in our cpp cfg for uncrustify.

FAKE_FUNCTION(Boo, RefType& (void));
FAKE_FUNCTION(Foo, (MyAwesomeType* (void)));
