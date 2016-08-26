// Fix doxygen support to include member groups

// See http://www.stack.nl/~dimitri/doxygen/manual/grouping.html#memgroup

// Note that the spec says three slashes, but their example has only two slashes.

// Once this is done, we can try turning on sp_cmt_cpp_start in Uncrustify.Common-CStyle.cfg.

/// Bucket allocator is used for allocations up to 64 bytes of memory.
/// It is represented by 4 blocks of a fixed-size "buckets" (for allocations of 16/32/48/64 bytes of memory).
/// Allocation is lockless, blocks are only growable.
class Class
{
public:
	///@{ Doxygen group 1
	virtual void*  Foo();
	virtual void*  Bar();
	///@}

	//@{ Doxygen group 2
	virtual void*  Foo();
	virtual void*  Bar();
	//@}
}
