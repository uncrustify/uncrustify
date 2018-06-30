
#define SOME_VAL1    ((MYINT)-1)
#define SOME_VAL2    (-2)
#define SOME_VAL3    -3
#define MULT(X, Y)    (X)*(Y)
#define SOME_JUNK    /*lint -e123 */ (const mytype *)-1

typedef (*my_fcn_ptr_t)(char *, int);
typedef (my_fcn_t)(char *, int);

void foo(void)
{
   uint crc = crc32_calc_full((const UINT8 *)"String", 6);

   crc = crc32_calc_full((const UINT8 *)&crc, sizeof(crc));

   a = (b) - 4;

   a = (UINT)-4;
   a = (UINT)+4;
   a = (UINT) * 4;
   a = (UINT) & 4;

   a = (uint32_t)-pb;
   a = (uint32_t) + pb;
   a = (uint32_t)*pb;
   a = (uint32_t)&pb;

   a = (Uint) - 4;
   a = (Uint) + 4;
   a = (Uint) * 4;
   a = (Uint) & 4;

   a = b * (int)flt;
   a = b * ((int)flt);

   a = b * (int)flt;
   a = b * (INT8)flt;
   a = b * (Uint)flt;

   a = *(int)&b;
   a = *(CHAR)&b;
   a = *(Uint) & b;

   a = (int)*pb;
   a = (CHAR)*pb;
   a = (Uint) * pb;

   a = (int)'a';
   a = (UINT8)'a';
   a = (Uint)'a';

   a = (int)*'a';
   a = (UINT8) * 'a';
   a = (Uint) * 'a';

   a = (int)*5;
   a = (UINT) * 5;
   a = (Uint) * 5;

   a = (int)*ape;
   a = (UINT)*ape;
   a = (Uint) * ape;

   a = (int)ape;
   a = (UINT)ape;
   a = (Uint)ape;

   a = (int)sizeof(x);
   a = (INT16)sizeof(x);
   a = (Uint)sizeof(x);

   a = (int)foo(x);
   a = (CHAR)foo(x);
   a = (Uint)foo(x);

   a = (int)(x);
   a = (CHAR)(x);
   a = (Uint)(x);

   a = (int)*(x);
   a = (CHAR)*(x);
   a = (Uint) * (x);

   a = (unsigned int)(1 + 4);
   a = (int)(1 + 1);
   a = (void *)(&str);
}

