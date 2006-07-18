typedef void (*my_fcn_ptr)(char *, int);
typedef const char *(my_fcn_ptr)(char *, int);
typedef int (my_fcn_ptr)(char *, int);
typedef struct foo *(my_fcn_ptr)(char *, int);
typedef const struct foo *(my_fcn_ptr)(char *, int);
typedef BOOL (my_fcn_ptr)(char *, int);

SMU foo(void)
{
   a = 4;
   return(SMUIFY(a));
}
