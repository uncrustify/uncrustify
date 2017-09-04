struct S { static if (false) void bar() { }; }

struct S { static if (false) { void bar() { }; } }
