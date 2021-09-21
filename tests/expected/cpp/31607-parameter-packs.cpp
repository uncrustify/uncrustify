template<typename ... A, int... B>
struct foo1 : foo1<A ..., (sizeof...(A)+B)...>
{
	foo1() {
		int x = sizeof...(A);
	}
};

template<int... X> int bar1()
{
	auto s = sizeof...(X);
	chomp(X)...;
	return X+...;
}

template<class R, typename ... Args>
void call1v(R (*fp)(Args ...));

template<class R, typename ... Args>
void call1p(R (*fp)(Args* ...));

template<class R, typename ... Args>
void call1r(R (*fp)(Args && ...));

template<class R, typename ... Args>
struct invoke1v : invoke<R (*)(Args ...)>
{
};

template<class R, typename ... Args>
struct invoke1p : invoke<R (*)(Args* ...)>
{
};

template<class R, typename ... Args>
struct invoke1r : invoke<R (*)(Args && ...)>
{
};

template  <  typename  ... A, int... B  >
struct foo2 :  foo2  <  A..., (  sizeof  ...  (  A  )  +  B  )  ...  >
{
	foo2() {
		int x = sizeof  ...  (  A  );
	}
};

template  <  int... X  > int bar2()
{
	auto s = sizeof  ...  (  X  );
	chomp(  X  )  ...;
	return X  +  ...;
}

template  <  class R, typename  ... Args  >
void call2v(  R (  *fp  )  (  Args ...  )  );

template  <  class R, typename  ... Args  >
void call2p(  R (  *fp  )  (  Args  * ...  )  );

template  <  class R, typename  ... Args  >
void call2r(  R (  *fp  )  (  Args && ...  )  );

template  <  class R, typename  ... Args  >
struct invoke2v :  invoke  <  R (  *  )  (  Args ...  )  >
{
};

template  <  class R, typename  ... Args  >
struct invoke2p :  invoke  <  R (  *  )  (  Args  * ...  )  >
{
};

template  <  class R, typename  ... Args  >
struct invoke2r :  invoke  <  R (  *  )  (  Args && ...  )  >
{
};
