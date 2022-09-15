#define gettext(Msgid) \
        QWERTY0 Msgid;
#ifdef TEST
   #define gettext1(Msgid)  \
           if (true)        \
           {                \
             QWERTY1 Msgid; \
           }                \
           while(true)      \
           {                \
             g = g + 1;     \
           }
int i = 3;

   #define x                \
           (                \
             x              \
           )                \
           {                \
             QWERTY2 Msgid; \
           }
void f()
{
	int i=2;
}
#endif

#pragma multi \
  line        \
  pragma

#define setint(x) x = x + 1
#define K 2
#define gettext2(Msgid)  \
        if (true)        \
        {                \
          QWERTY1 Msgid; \
          do             \
          {              \
            g = g + 1;   \
          }              \
          while(true);   \
        }

#warning multi \
  line         \
  warning

#ifdef TEST1
   #ifdef TEST2
      #ifdef TEST3
         #ifdef TEST4
            #define gettext1(Msgid)  \
                    if (true)        \
                    {                \
                      QWERTY1 Msgid; \
                      do             \
                      {              \
                        if (false) { \
                          g = g + 1; \
                        }            \
                      }              \
                      while(true);   \
                    }
         #endif
      #endif
   #endif
#endif
