void func(void)
{
	switch (cond)
	{
	case CASE_F:
		synchronized(thingy)
		{
			do_a();
			do_b();
		}
		break;
	}

	synchronized(thingy)
	{
		do_a();
		do_b();
	}
}
