// Asm blocks have a few ways of being opened and closed depending on the compiler. Detect and ignore the contents, including alignment.

// * Fucked up \_\_asm\_\_ quoted string stuff in AtomicQueue.cpp (plus embedded \t's) and indentation

// Workaround: can always fall back on disable/enable_processing_cmt.

void foo()
{
	__asm__ __volatile__
	(
	"0:\n\t"
		"bar	%0, [%4]\n\t"
	"1:\n\t"
	);
}
