// Test: align_func_proto_span with several prototypes on one line:
// only the first one of a line is aligned, the others must not be glued
// to their return type

struct A
{
	int           g(); int hh();
	unsigned long k(); bool m();
	A(int); A(long, long);
	int get() const {
		return v;
	}
	void set(int x) {
		v = x;
	}
	long size() const;
};

int  a(); unsigned bb();
void c(); double dd();
long e();

struct B
{
	int x; long yy();
	short z(); int ww;
};

// The return type of a prototype taken for a variable by the tokenizer
struct C
{
	unsigned long           long_name();
	static    std::string&& get_r();
	int                     x();
	constexpr T&&           get_t();
};

// A newline in a parameter list does not end the line
long q(int a,
       int b); void r();
int  longer_name();

void vars(void)
{
	int  a = foo(1,
	             2); int b;
	long cc = 3;
	int  x2 = bar({1,
	               2}); int yy;
	long c3 = 4;
}
