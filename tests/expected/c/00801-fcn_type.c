typedef void (*my_fcn_ptr)(char *, int);
typedef const char *(my_fcn_ptr)(char *, int);
typedef int (my_fcn_ptr)(char *, int);
typedef struct foo *(my_fcn_ptr)(char *, int);
typedef enum foo *(*my_fcn_ptr)(char *, int);
typedef const struct foo *(*my_fcn_ptr)(char *, int);
typedef BOOL (my_fcn_ptr)(char *, int);
typedef INT32 (*my_fcn_ptr)(char *, int);
typedef int INT32;
typedef struct foo
{
   int a;
} fooey;

typedef struct
{
   int a;
} queso;

struct myfoo
{
   INT32            age;
   const struct foo *(*my_fcn_ptr)(char *, int);
   const CHAR       *name;
   MYTYPE           (*foo)(int, char);
   void             *user;
   void             (*foo)(int, char);
};

SMU foo(void)
{
   double AAA = 1.e-3, BBB = 0.016, CCC = 2 * DDD * sqrt(EEE);

   a = 4;
   (*ABC)();
   return(SMUIFY(a));
}

typedef struct
{
   void (*newObject)(const object_info *info, const IObject **interface, struct object_h *instance);
} IObjectFactory;

