#define MY_DEF(Type, ...) \
	enum Type {  \
		__VA_ARGS__, \
	};
