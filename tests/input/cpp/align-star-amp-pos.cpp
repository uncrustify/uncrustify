
/** First, the typedefs */
typedef int MY_INT;
typedef int * MY_INTP;
typedef int & MY_INTR;
typedef CFoo& foo_ref_t;
typedef int(*foo_t)(void *bar);
typedef const char *(*somefunc_t)(void *barstool);

/* Now, the types */
struct foo1 {
 unsigned int d_ino;
 const char *d_reclen;
 unsigned short d_namlen;
 char d_name[1];
 CFoo&fref;
};

struct foo { int a; char *b };

static   int idx;
static   const char **tmp;
 CFoo&fref;

static   char buf[64];
static unsigned long how_long;
// comment
static   int **tmp;
static   char buf[64];


void bar(int someval,
         void *puser,
         const char *filename,
         struct willy *the_list,
         int list_len)
{
   int idx;
   const char **tmp;
   char buf[64];
   CFoo&fref;

   unsigned long how_long;

   return(-1);
}

