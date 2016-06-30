#define inline_2 __forceinline
#define inline(i) inline_##i
#define foo(x) inline(2) x()
#define PLD(reg,offset)    pld    [reg, offset]  \
	pld    [reg, offset] \
	pld     [reg, offset]
