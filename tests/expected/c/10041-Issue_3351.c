#include <stdio.h>

void foo(unsigned flags, unsigned COMMENT) {
	/* Discard file comment if any */
	if ((flags & COMMENT) != 0) {
		while (getchar() != 0) /* null */
		{
		}
	}
}
