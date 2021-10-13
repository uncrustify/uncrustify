#define foo1(x) { return  x; }
#define foo2(x) { return(x); }
#define foo3(x) { return  (x); }
#define foo4(x) { return{x}; }
#define foo5(x) { return  {x}; }
#define foo6(x) { return /**/x; }

#define case1(x) return  x
#define case2(x) return(x)
#define case3(x) return  (x)
#define case4(x) return{x}
#define case5(x) return  {x}
#define case6(x) return /**/x

void foo(int x)
{
	switch (x)
	{
	case 1:
		return  1;
	case 2:
		return(2);
	case 3:
		return  (3);
	case 4:
		return{4};
	case 5:
		return  {5};
	case 6:
		return /**/6;
	default:
		return;
	}
}
