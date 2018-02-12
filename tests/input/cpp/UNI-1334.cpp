// This should not be screwing with the trailing backslash and indentation of contents!
// unless it's on the first line where it's controlled by sp_before_nl_cont which we have set on add.
// Devs should expect misalignment of the nl_cont tokens because we're not messing with the nl_cont from the define body.

#define MY_DEFINE(param1, param2)\
	my_long_foo_function(param1);\
	bar(param2);
