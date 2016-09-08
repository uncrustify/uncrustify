#define a	z	\
			x

#define a(b)	z	\
				x

#define ab(b)	z	\
				x

#define abc(b)	z	\
				x

#define abcd(b)	z	\
				x


#if FOO
#	define D(a, ...) B(FOO(a, __LINE__, __VA_ARGS__))
#	define C(msg)			\
		PP_WRAP_CODE(		\
			if (!msg)		\
			{				\
				BAR();		\
				BARBAR();	\
				BARBARBAR();\
			})
#else
#	define C(msg, ...)		EMPTY
#endif
