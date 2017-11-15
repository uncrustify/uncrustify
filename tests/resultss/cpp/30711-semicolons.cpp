static int foo(int bar);

static int foo(int bar)
{
	for (;;)
	{
		break;
	}
	if (a)
	{
		foo();
	}

	if (b)
		if (c)
			bar();
		else
			;

	else
	{
		foo();
	}
	switch (a)
	{
	case 1: break;
	case 2: break;
	default: break;
	}
	while (b-->0)
	{
		bar();
	}
	do
	{
		bar();
	} while (b-->0  );
}

enum FPP {
	FPP_ONE = 1,
	FPP_TWO = 2,
};

struct narg {
	int abc;
	char def;
	const char *ghi;
};

class CFooRun {
long stick();
int bar() {
	m_ick++;
}

CFooRun();
~CFooRun() {
}
};

void f()
{
	if (tmp[0] == "disk")
	{
		tmp = split (tmp[1], ",");
		DiskEntry entry = { tmp[0], tmp[2],
			            stxxl::int64 (str2int (tmp[1])) *
			            stxxl::int64 (1024 * 1024) };
		disks_props.push_back (entry);
	}
}

template < class > struct type;

template < class T >
class X {
typedef type < T > base;
void f () {
	( base :: operator * () );
}
};

namespace N
{
class C
{
#define NOP(x) { \
}
};
}

namespace N
{
class C
{
};
}

void deallocate2(S **s_ptr)
{
	{
		void *stopper_for_apply = (int[]){0};
		void **list_for_apply = (void *[]){(*s_ptr)->arr, *s_ptr, stopper_for_apply};
		for (int i = 0;list_for_apply[i] != stopper_for_apply;i++) {
			saferFree((void *) &(list_for_apply[i]));
		}
	}
}
