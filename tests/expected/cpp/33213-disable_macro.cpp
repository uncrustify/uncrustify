#include <stdio.h>

// this macro should NOT be modified ...
#define CHK(...)   \
  do               \
  {                \
      a+=1;        \
        a=b=0;     \
          c<<1;    \
  } while (0+0)


// ... whereas this should be indented and formatted
int main()
{
	int a,b,c = 0;
	if (a < c)
	{
		c += 1;
	}
	a = b = 0;
	c << 1;
	CHK;
}
