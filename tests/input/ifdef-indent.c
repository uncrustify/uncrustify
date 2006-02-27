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
}

