void asd(void)
{
	a < up_lim() ? do_hi() : do_low;
	a[ a<b>c] = d;
}

[[nodiscard]] inline static CFErrorRef _Nullable CreateErrorIfError(CFStringRef const inDomain, CFIndex const inCode, CFDictionaryRef const inInformation) {
	[[maybe_unused]] auto const [iterator, inserted]{ super_type::insert(ioFileReference) };
	if (inCode == 0) {
		return nullptr;
	}
	return ::CFErrorCreate(kCFAllocatorDefault, inDomain, inCode, inInformation);
}

[[gnu::always_inline]] [[gnu::hot]] [[gnu::const]] [[nodiscard]]
inline int f();
[[gnu::always_inline, gnu::const, gnu::hot, nodiscard]]
int f();
[[using gnu : const, always_inline, hot]] [[nodiscard]]
int f [[gnu::always_inline]]();

int f(int i) [[expects: i > 0]] [[ensures audit x: x < 1]];

void f() {
	int i [[cats::meow([[]])]];
	int x [[unused]] = f();
}

int f(int i) [[deprecated]] {
	switch(i) {
	case 1: [[fallthrough]];
		[[likely]] case 2: return 1;
	}
	return 2;
}

[[
unused, deprecated("keeping for reference only")
]]
void f()
{
}

[[noreturn]] void f() [[deprecated("because")]] {
	throw "error";
}

void print2(int * [[carries_dependency]] val)
{
	std::cout<<*p<<std::endl;
}

class X {
public:
int v() const {
	return x;
}
int g() [[expects: v() > 0]];
private:
int k() [[expects: x > 0]];
int x;
};

class [[foo, bar("baz")]] /**/ Y : private Foo, Bar {
public:
int v(int &x) {
	return x;
}
};

class
	[[foo]]
	[[bar("baz")]]
	Z : Foo, public Bar {
public:
int v(int * x) {
	return *x;
}
};

int g(int* p) [[ensures: p != nullptr]]
{
	*p = 42;
}

bool meow(const int&) {
	return true;
}
void i(int& x) [[ensures: meow(x)]]
{
	++x;
}

enum Enum {
	a, b
};
enum class [[foo]] Enum {
	a, b
};
enum struct [[foo]] /**/ [[bar("baz")]] Enum {
	a, b
};
enum [[foo]]
Enum {
	a, b
};
enum class [[foo]] //
[[bar("baz")]] Enum {
	a, b
};
enum struct //
[[bar("baz")]] Enum {
	a, b
};
enum
[[foo]] [[bar("baz")]] /**/ Enum {
	a, b
};
enum class /**/ [[foo]] [[bar("baz")]]
Enum {
	a, b
};
enum //
struct
[[foo]]
[[bar("baz")]]
Enum {
	a, b
};
