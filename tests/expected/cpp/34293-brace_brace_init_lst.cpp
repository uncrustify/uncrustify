int main()
{
	int a0[][] = {{1}};
	unknown_type b0 = {{2}};
	auto c0 = unknown_type  {{3}};
	auto d0 = func( {{3}}  );
	auto e0 = func( unknown_type  {{3}}  );

	int a1[][] = {{1}};
	unknown_type b1 = {{2}};
	auto c1 = unknown_type{{3}};
	auto d1 = func({{3}});
	auto e1 = func(unknown_type{{3}});

	int a2[][] = {{1}};
	unknown_type b2 = {{2}};
	auto c2 = unknown_type{{3}};
	auto d2 = func({{3}});
	auto e2 = func(unknown_type{{3}});

	return 1;
}
