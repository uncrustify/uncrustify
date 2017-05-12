// Uncrustify does not process the intention of an using alias,
// unknown_kw will therefore no be parsed as known keyword
using unknown_kw = int;

int main()
{
	// 'int' is a known c++ keyword
	auto a0 = int {  1  };
	auto b0 = unknown_kw {  2  };
	auto c0 = ::unknown_kw {  3  };
	auto d0 = (int) unknown_kw {  4  };
	auto e0 = (int) ::unknown_kw {  5  };
	auto f0 = static_cast<int>(unknown_kw {  6  });
	auto g0 = static_cast<int>(::unknown_kw {  7  });

	auto a1 = int {1};
	auto b1 = unknown_kw {2};
	auto c1 = ::unknown_kw {3};
	auto d1 = (int) unknown_kw {4};
	auto e1 = (int) ::unknown_kw {5};
	auto f1 = static_cast<int>(unknown_kw {6});
	auto g1 = static_cast<int>(::unknown_kw {7});



	auto a2 = int

	{1};
	auto b2 = unknown_kw

	{2};
	auto c2 = ::unknown_kw

	{3};
	auto d2 = (int) unknown_kw

	{4};
	auto e2 = (int) ::unknown_kw

	{5};
	auto f2 = static_cast<int>(unknown_kw

	                           {6});
	auto g2 = static_cast<int>(::unknown_kw

	                           {7});



	auto a1 = int {

		1

	};
	auto b1 = unknown_kw {

		2

	};
	auto c1 = ::unknown_kw {

		3

	};
	auto d1 = (int) unknown_kw {

		4

	};
	auto e1 = (int) ::unknown_kw {

		5

	};
	auto f1 = static_cast<int>(unknown_kw {

		6

	});
	auto g1 = static_cast<int>(::unknown_kw {

		7

	});

	return 1;
}