typedef short (* hello1) (char         coolParam,
               ushort *,
               unsigned int anotherone);

short (* hello2)(char         coolParam,
               ulong *,
	uchar,
               unsigned int anotherone);

short hello3 (char         coolParam,
               ushort *,
               unsigned int anotherone);

void x (custom_t * e, void (*funcptr) );
void x (custom_t * e, void (*funcptr)[] );
void x (custom_t * e, void (*funcptr)(int, int) );
void x (custom_t * e, void (*funcptr)(int, int)[] );

