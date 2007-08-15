struct foo {
 unsigned int d_ino;
 /* Comment */
 unsigned short d_reclen;
 unsigned short d_namlen;
 /* Comment */
 char d_name[1];
};

struct foo
{
   /* Comment */
 unsigned int d_ino;
 unsigned short d_reclen;
 unsigned short d_namlen;
 /* Comment */
 char d_name[1];
};

struct foo { int a; char *b };

