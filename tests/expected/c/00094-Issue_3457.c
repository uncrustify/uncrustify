#define IS_UNSIGNED(t) \
	_Generic((t), \
		 uint8_t: true, \
		 uint16_t: true, \
		 uint32_t: true, \
		 uint64_t: true, \
		 unsigned long long: true, \
		 int8_t: false, \
		 int16_t: false, \
		 int32_t: false, \
		 int64_t: false, \
		 signed long long: false)
