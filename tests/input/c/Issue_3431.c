int add(
int a,
int b
#define HAVE_C
#ifdef HAVE_C
, int c
#endif
)
{
int sum = a + b;
#ifdef HAVE_C
sum += c;
#endif
return sum;
}
