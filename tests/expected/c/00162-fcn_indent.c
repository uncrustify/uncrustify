int
this_is_a_function_proto(int a,
                         char * b);

int this_is_a_function_def(int a,
	char * b)
{
	this_is_a_function_call(a,
	                        b);

	a = another_function_call(a,
	                          b);

}

typedef const char * pu8_t;

typedef short (*hello1)(char coolParam,
                        ushort *,
                        unsigned int anotherone);

typedef const unsigned char * (getfcn_t)(
	int idx, ulong op);

short (*hello2)(char coolParam,
                ulong *,
                uchar,
                unsigned int anotherone);

const unsigned char * (*getstr) (
	int idx,
	ulong op);

short
hello3 (char coolParam,
        ushort *,
        unsigned int anotherone);
