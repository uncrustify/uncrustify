#include <vector>
#include <algorithm>

using some_type = int;
namespace Ns {
using some_type = int;
}

class BracedInitListBase {
public:
BracedInitListBase()
	: a{int{1}},
	b(int(some_type(1))),
	c(int{some_type(1)}),
	d{int(some_type(1))},
	e{some_type{some_type{a}}}
{
}

virtual int getA() const {
	return a;
}
private:
int a{};
int b{1};
int c = {1};
int d = int{1};
some_type e{1};
some_type f = {1};
some_type g = some_type{1};
std::vector<some_type> h{some_type{4}, 5};
::std::vector<some_type> i = ::std::vector<some_type>{4, some_type{5}};
some_type j = ::std::vector<some_type>{4, some_type{5}}[1];
some_type k[2]{1, 2};
some_type l[2] = {1, 2};

union SomeUnion {
	int a;
	some_type b{};
};
};

class BracedInitListDerived : public BracedInitListBase {
public:
int getA() const override {
	return BracedInitListBase::getA();
}
};

some_type inc(some_type a)
{
	return some_type{++a};
}

some_type sum(some_type a, some_type b = some_type{1})
{
	return a + inc(some_type{b - some_type{1}});
}

void braced_init_list_int()
{
	{
		int a{};
		int b = {};
		int c = int{};
		int d = int{int{}};
		int{};
		int{int{}};
	}
	{
		int a{1};
		int b = {1};
		int c = int{1};
		int d = int{int{1}};
		int{1};
		int{int{1}};
	}
}

void braced_init_list_some_type()
{
	{
		some_type a{};
		some_type b = {};
		some_type c = some_type{};
		some_type d = some_type{some_type{}};
		some_type{};
		some_type{some_type{}};
	}
	{
		some_type a{1};
		some_type b = {1};
		some_type c = some_type{1};
		some_type d = some_type{some_type{1}};
		some_type{1};
		some_type{some_type{1}};
	}
	{
		::some_type a{1};
		::some_type b = {1};
		::some_type c = ::some_type{1};
		::some_type d = ::some_type{::some_type{1}};
		::some_type{1};
		::some_type{::some_type{1}};
	}
	{
		Ns::some_type a{1};
		Ns::some_type b = {1};
		Ns::some_type c = Ns::some_type{1};
		Ns::some_type d = Ns::some_type{Ns::some_type{1}};
		Ns::some_type{1};
		Ns::some_type{Ns::some_type{1}};
	}
	{
		::Ns::some_type a{1};
		::Ns::some_type b = {1};
		::Ns::some_type c = ::Ns::some_type{1};
		::Ns::some_type d = ::Ns::some_type{::Ns::some_type{1}};
		::Ns::some_type{1};
		::Ns::some_type{::Ns::some_type{1}};
	}
}

void braced_init_list_some_type_auto()
{
	{
		auto b = some_type{};
		auto c = some_type{some_type{}};
	}
	{
		auto a = {1};
		auto b = some_type{1};
		auto c = some_type{some_type{1}};
	}
	{
		auto b = ::some_type{1};
		auto c = ::some_type{::some_type{1}};
	}
	{
		auto b = Ns::some_type{1};
		auto c = Ns::some_type{Ns::some_type{1}};
	}
	{
		auto b = ::Ns::some_type{1};
		auto c = ::Ns::some_type{::Ns::some_type{1}};
	}
}

void braced_init_list_function_call()
{
	{
		some_type a{sum(some_type{}, some_type{})};
		some_type b = sum(some_type{}, some_type{});
		some_type c = some_type{sum(some_type{}, some_type{})};
		some_type{sum(some_type{}, some_type{})};
		some_type{some_type{sum(some_type{}, some_type{})}};
	}
	{
		some_type a{sum(some_type{1}, some_type{1})};
		some_type b = sum(some_type{1}, some_type{1});
		some_type c = some_type{sum(some_type{1}, some_type{1})};
		some_type{sum(some_type{a}, some_type{b})};
		some_type{some_type{sum(some_type{a}, some_type{b})}};
	}
	{
		::some_type a{sum(::some_type{1}, ::some_type{1})};
		::some_type b = sum(::some_type{1}, ::some_type{1});
		::some_type c = ::some_type{sum(::some_type{1}, ::some_type{1})};
		::some_type{sum(::some_type{a}, ::some_type{b})};
		::some_type{::some_type{sum(::some_type{a}, ::some_type{b})}};
	}
	{
		Ns::some_type a{sum(Ns::some_type{1}, Ns::some_type{1})};
		Ns::some_type b = sum(Ns::some_type{1}, Ns::some_type{1});
		Ns::some_type c = Ns::some_type{sum(Ns::some_type{1}, Ns::some_type{1})};
		Ns::some_type{sum(Ns::some_type{a}, Ns::some_type{b})};
		Ns::some_type{Ns::some_type{sum(Ns::some_type{a}, Ns::some_type{b})}};
	}
	{
		::Ns::some_type a{sum(::Ns::some_type{1}, ::Ns::some_type{1})};
		::Ns::some_type b = sum(::Ns::some_type{1}, ::Ns::some_type{1});
		::Ns::some_type c = ::Ns::some_type{sum(::Ns::some_type{1}, ::Ns::some_type{1})};
		::Ns::some_type{sum(::Ns::some_type{a}, ::Ns::some_type{b})};
		::Ns::some_type{::Ns::some_type{sum(::Ns::some_type{a}, ::Ns::some_type{b})}};
	}
}

void braced_init_list_function_call_newline()
{
	{
		some_type a{
			sum(some_type{},
			    some_type{}
			    )
		};
		some_type b = sum(
			some_type{}, some_type{});
		some_type c = some_type{
			sum(
				some_type{}, some_type{})};
		some_type
		{sum
			 (some_type{},
			 some_type{}
			 )
		};
		some_type
		{some_type{sum
			           (some_type{}, some_type{})}};
	}
}

void braced_init_list_array()
{
	{
		some_type a[]{};
		some_type b[] = {};
		some_type c[] = {{}, {}};
	}
	{
		some_type a[]{1, 2};
		some_type b[] = {1, 2};
		some_type c[] = {some_type{1}, some_type{2}};
	}
}

void braced_init_list_template()
{
	{
		std::vector<some_type> a{};
		std::vector<some_type> b = {};
		std::vector<some_type> c = {{}, {}};
		std::vector<some_type> d = std::vector<some_type>{};
		std::vector<some_type> e = std::vector<some_type>{{}, {}};
		std::vector<some_type> f = std::vector<some_type>{some_type{}, some_type{}};
		std::vector<some_type>{};
		std::vector<some_type>{{}, {}};
		std::vector<some_type>{some_type{}, some_type{}};
	}
	{
		std::vector<some_type> a{1, 2};
		std::vector<some_type> b = {1, 2};
		std::vector<some_type> c = std::vector<some_type>{1, 2};
		std::vector<some_type> d = std::vector<some_type>{some_type{1}, some_type{2}};
		std::vector<some_type>{1, 2};
		std::vector<some_type>{some_type{1}, some_type{2}};
	}
}

void braced_init_list_lambda()
{
	std::vector<some_type> a{1, 2};
	some_type b{2};

	auto c = [] {
			 return true;
		 };
	auto d = [](){
			 return true;
		 };

	std::find_if(a.begin(), a.end(), [&b](const some_type &v){
		return v == b;
	});
	std::find_if(a.begin(), a.end(), [](const some_type &v){
		some_type b{2}; return v == b;
	});
}
