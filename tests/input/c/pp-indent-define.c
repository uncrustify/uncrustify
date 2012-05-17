
#pragma do not change anything in this pragma!

// This next bit should parse as '#', pragma, preproc-body, nl-cont, 
//   preproc-body, nl-cont, preproc-body
#pragma multi \
   line \
      pragma

#pragma mark -------- Protected Member Functions ----------------

#pragma some comment follows    // comment

#error Define EVERYTHING_OK if you would like to compile your code \
  or not if you would like to stop!

#if !defined(EVERYTHING_OK)
#error Define EVERYTHING_OK if you would like to compile your code \
  or not if you would like to stop!
#endif

#define macro

#define \
macro \
 a = 1

#define macro \
\
 a = 1

#define macro() \
do \
{ \
int i; \
\
i = a^2; \
*b=a; \
} \
while(0)


#define macro( \
\
a, \
b \
) \
do \
{ \
int i; \
\
i = a^2; \
*b=a; \
} \
while(0)

#define macro( \
\
a, \
b \
) \
x++; \
do \
{ \
int i; \
\
i = a^2; \
*b=a; \
} \
while(0)

#define macro( \
\
a, \
b \
) \
do \
{ \
int i; \
\
i = a^2; \
*b=a; \
} \
while(0); \
x++

#define macro( \
\
a, \
b \
) \
do \
{ \
int i; \
\
do \
{ \
int i; \
\
i = a^2; \
*b=a; \
} \
while(0); \
i = a^2; \
*b=a; \
} \
while(0)

#define macro( \
\
a, \
b \
) \
do \
{ \
do \
{ \
int i; \
\
i = a^2; \
*b=a; \
} \
while(0);\
} \
while(i--)

void function(
int a,
int b
)
{
int x;
out=(
in + 1
);
if (D)
{
int y;
out++;
}
}

#define macro( \
\
a, \
b \
) \
do \
{ \
do \
{ \
int i; \
\
i = a^2; \
*b=a; \
} \
while(0)

#define macro() \
do \
{ \
int i;

#define macro() \
i=a^2; \
*b=a; \
} \
while(0)


#if TEST

#define macro

#define \
macro \
 a = 1

#define macro \
\
 a = 1

#define macro() \
do \
{ \
int i; \
\
i = a^2; \
*b=a; \
} \
while(0)


#define macro( \
\
a, \
b \
) \
do \
{ \
int i; \
\
i = a^2; \
*b=a; \
} \
while(0)

#define macro( \
\
a, \
b \
) \
x++; \
do \
{ \
int i; \
\
i = a^2; \
*b=a; \
} \
while(0)

#define macro( \
\
a, \
b \
) \
do \
{ \
int i; \
\
i = a^2; \
*b=a; \
} \
while(0); \
x++

#define macro( \
\
a, \
b \
) \
do \
{ \
int i; \
\
do \
{ \
int i; \
\
i = a^2; \
*b=a; \
} \
while(0); \
i = a^2; \
*b=a; \
} \
while(0)

#define macro( \
\
a, \
b \
) \
do \
{ \
do \
{ \
int i; \
\
i = a^2; \
*b=a; \
} \
while(0);\
} \
while(i--)

#define macro( \
\
a, \
b \
) \
do \
{ \
do \
{ \
int i; \
\
i = a^2; \
*b=a; \
} \
while(0)

#define macro() \
do \
{ \
int i;

#define macro() \
i=a^2; \
*b=a; \
} \
while(0)

#endif

