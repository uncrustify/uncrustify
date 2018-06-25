lass A
{
	void f0(void);

	template<typename T, typename U>
	void g(T s, U t)
	{
		return;
	}
	void f1(void);

	template
	<typename T,
	 typename U>
	void h(T s, U t)
	{
		return;
	}
	void f2(void);

	template
	<typename T,
	 typename U>
	void
	i(T s, U t)
	{
		return;
	}
	void f3(void);

	template
	<typename T,
	 typename U>
	void
	j
	        (T s, U t)
	{
		return;
	}
	void f4(void);

	template
	<typename T,
	 typename U>
	void
	k
	(
		T s, U t)
	{
		return;
	}
	void f5(void);

	template
	<typename T,
	 typename U>
	void
	l
	(
		T s,
		U t
	)
	{
		return;
	}
}
