void main()
{
	while (*stringcur)
	{
#ifdef NO8BIT
		if (((*bufcur++ ^ *stringcur) & 0x7F) != 0)
#else /* NO8BIT */
		if (*bufcur++ != *stringcur)
#endif /* NO8BIT */ /* Issue #3034 */
                        break;
	}
}

