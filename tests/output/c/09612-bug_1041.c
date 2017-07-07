void (*g_func_table[32])(void) = {
	[0 ... 31] = func_dummy,
	[0] = func_0,
	[1] = func_1,
	[2] = func_2,
};
