#define VTABLE_DECLARE                                            \
	extern struct vtable_struct_name_macro vtable_base_macro; \
	struct vtable_struct_name_macro

#define VTABLE_METHOD(retvalue, method, args ...) \
	retvalue(*method)(args)

VTABLE_DECLARE {
	VTABLE_METHOD(int, get, const char *name);
	VTABLE_METHOD(int, set, const char *name, int value);
};
