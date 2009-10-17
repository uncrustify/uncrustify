int this_is_a_function_proto(int a,
char * b);

int this_is_a_function_def(int a,
char * b)
{
this_is_a_function_call(a,
b);

a = another_function_call(a,
b);

}

typedef short (*hello1)(char         coolParam,
          ushort *,
       unsigned int anotherone);

short (*hello2)(char         coolParam,
          ulong *,
	uchar,
               unsigned int anotherone);

short hello3 (char         coolParam,
        ushort *,
              unsigned int anotherone);
