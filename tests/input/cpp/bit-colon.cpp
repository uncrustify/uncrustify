class C
{
   size_t f1 : 1;
   size_t f2 : sizeof(size_t) - 1;
};

struct S
{
   size_t f1 : 1;
   size_t f2 : sizeof(size_t) - 1;
};
