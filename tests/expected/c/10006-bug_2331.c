#define SET_STACK(stack)            \
	do {                            \
		__asm__ __volatile__ (      \
			"mov S, %[oper]"        \
			:                       \
			: [oper] "r" (stack)    \
			: "S"                   \
			);                      \
	} while (0)

int somearray[10];
