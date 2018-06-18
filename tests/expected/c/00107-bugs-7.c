
struct somestruct *
mult2(int val);

somestruct *
dumb_func(int val);



struct somestruct *
mult2(int val)
{
   int a;

   a = val + (foo * bar);

   a = val + (bar);

   a = val + (CFoo::bar_t)7;

   a = val + (myfoo.size);

   return(NULL);
}

