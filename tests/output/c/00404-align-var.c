struct foo1
{
   unsigned int   d_ino;
   const char    *d_reclen;
   unsigned short d_namlen;
   char d_name[1];
};

struct foo2
{
   unsigned int a   : 1;
   unsigned int bcd : 3;
   unsigned int ef  : 2;
   unsigned int     : 2;

   unsigned short more;

   int fields;
};

typedef struct
{
   bitfld a : 8;
   bitfld b : 16;
   bitfld   : 8;
} type_t;

struct foo { int a; char *b };

static int          idx;
static const char **tmp;

static char          buf[64];
static unsigned long how_long;
// comment
static int **tmp;
static char  buf[64];


void bar(int           someval,
         void         *puser,
         const char   *filename,
         struct willy *the_list,
         int           list_len)
{
   int          idx;
   const char **tmp;
   char         buf[64];

   unsigned long how_long;

   return(-1);
}

