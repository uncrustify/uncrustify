
const char *token_names[] =
{
   [CT_POUND]        = "POUND",
   [CT_PREPROC]      = "PREPROC",
   [CT_PREPROC_BODY] = "PREPROC_BODY",
   [CT_PP]           = "PP",
};


int main(int argc, char *argv[])
{
   struct junk a[] =
   {
      { "version",  0,   0,   0 },
      { "file",     1, 150, 'f' },
      { "config",   1,   0, 'c' },
      { "parsed",  25,   0, 'p' },
      { NULL,       0,   0,   0 }
   };
}


color_t colors[] =
{
   { "red",   { 255, 0,   0 } }, { "blue",   {   0, 255, 0 } },
   { "green", {   0, 0, 255 } }, { "purple", { 255, 255, 0 } },
};


struct foo_t bar =
{
   .name = "bar",
   .age  = 21
};


struct foo_t bars[] =
{
   [0] = { .name = "bar",
           .age  = 21 },
   [1] = { .name = "barley",
           .age  = 55 },
};

void foo(void)
{
   int  i;
   char *name;

   i    = 5;
   name = "bob";
}

/**
 * This is your typical header comment
 */
int foo(int bar)
{
   int idx;
   int res = 0;      // trailing comment
                     // that spans two lines
   for (idx = 1; idx < bar; idx++)
      /* comment in virtual braces */
      res += idx;

   res *= idx;        // some comment

   // almost continued, but a NL in between

// col1 comment in level 1
   return(res);
}

// col1 comment in level 0


#define foobar(x)             \
   {                          \
      for (i = 0; i < x; i++) \
      {                       \
         junk(i, x);          \
      }                       \
   }


void foo(void)
{
   switch(ch)
   {
   case 'a':
      {
         handle_a();
         break;
      }

   case 'b':
      handle_b();
      break;

   case 'c':
   case 'd':
      handle_cd();
      break;

   case 'e':
      {
         handle_a();
      }
      break;

   default:
      handle_default();
      break;
   }
}

