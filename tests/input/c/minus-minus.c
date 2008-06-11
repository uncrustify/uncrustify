int a = 5;
int *b = &a;
int negneg(void)
{
   return - -a;
}

int pospos(void)
{
   return + +a;
}

int negpos(void)
{
   return - +a;
}

int posneg(void)
{
   return + -a;
}

#if VERSION < -5
int negnegneg(void)
{
   return - - -a;
}
#else
int negat(void)
{
   return - *b;
}
#endif
