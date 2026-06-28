class MyClass {};

void test_empty_lines_nonstatic(MyClass &obj)
{
	obj.method(param1 + param1, param2, param3);

	obj.method(p41,             p5,     p6);


	obj.method(p711, p8, p9);



	obj.method(p7111, p8, p9);
}

void test_comment_lines_nonstatic(MyClass &obj)
{
	obj.method(param1 + param1, param2, param3);
	// comment line
	obj.method(p41,             p5,     p6);
	// comment line A
	/* comment line B */
	obj.method(p711,            p8,     p9);
	// comment line A
	// comment line B
	// comment line C
	obj.method(p7222,           p8,     p9);
}

void test_pp_lines_nonstatic(MyClass &obj)
{
	obj.method(param1 + param1, param2, param3);
#ifdef FEATURE
	obj.method(p41,             p5,     p6);
#endif
#ifdef FEATURE2
	obj.method(p711, p8, p9);
#endif
#ifdef FEATURE3
#ifdef FEATURE4
	obj.method(p7222, p8, p9);
#endif
#endif
}

void test_mixed_lines_nonstatic(MyClass &obj)
{
	obj.method(param1 + param1, param2, param3);

#ifdef FEATURE
	obj.method(p41,             p5,     p6);
#endif
	// comment line A
	// comment line B

	obj.method(p711,            p8,     p9);

#ifdef FEATURE2
	// comment line C
	obj.method(p7222,           p8,     p9);
#endif
}

void test_empty_lines_static(void)
{
	MyClass::method(param1 + param1, param2, param3);

	MyClass::method(p41,             p5,     p6);


	MyClass::method(p711, p8, p9);



	MyClass::method(p7111, p8, p9);
}

void test_comment_lines_static(void)
{
	MyClass::method(param1 + param1, param2, param3);
	// comment line
	MyClass::method(p41,             p5,     p6);
	// comment line A
	/* comment line B */
	MyClass::method(p711,            p8,     p9);
	// comment line A
	// comment line B
	// comment line C
	MyClass::method(p7222,           p8,     p9);
}

void test_pp_lines_static(void)
{
	MyClass::method(param1 + param1, param2, param3);
#ifdef FEATURE
	MyClass::method(p41,             p5,     p6);
#endif
#ifdef FEATURE2
	MyClass::method(p711, p8, p9);
#endif
#ifdef FEATURE3
#ifdef FEATURE4
	MyClass::method(p7222, p8, p9);
#endif
#endif
}

void test_mixed_lines_static(void)
{
	MyClass::method(param1 + param1, param2, param3);

#ifdef FEATURE
	MyClass::method(p41,             p5,     p6);
#endif
	// comment line A
	// comment line B

	MyClass::method(p711,            p8,     p9);

#ifdef FEATURE2
	// comment line C
	MyClass::method(p7222,           p8,     p9);
#endif
}
