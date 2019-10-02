#define UNC_DECLARE_FLAGS(flag_type, enum_type) \
	using flag_type = flags<enum_type>

#define UNC_DECLARE_OPERATORS_FOR_FLAGS(flag_type)                        \
	inline flag_type operator&(flag_type::enum_t f1, flag_type::enum_t f2) \
	{ return(flag_type { f1 } & f2); }
