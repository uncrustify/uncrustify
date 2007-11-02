#ifdef foo
#include <foo1.h>
#else
#include <foo2.h>
#endif

int
show_interrupts(struct seq_file *p, void *v)
{
#ifndef CONFIG_SMP
	a++;
#else
	for (b = 0; b < 9; b++)
		if (b & 1)
			k++;
#endif

    if (v)
    {
       bar(v);
       #if DEBUG == 1
       printf("yup\n");
       #endif
    }
}

void foo()
{
int i=0;
#if DEBUG == 1
i--;
#endif
i++;
}

