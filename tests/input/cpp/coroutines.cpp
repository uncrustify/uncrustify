Awaiter operator co_await(Other const &o);
Awaiter Awaitable::operator co_await() const noexcept
{
   co_return {};
}

T f(int *q, int x, int a, int b)
{
   co_return;
   co_return 42;
   co_return a + b;
   co_return (x);
   co_return(x);
   co_return {1, 2};
   co_return{3, 4};
   co_return *q;
   co_return convert<int>(1);
   co_return ::ns::value;
   co_return co_await get();
   co_return (co_await get());
   auto  y = co_await get();
   auto &r = co_await ref();
   auto *p = co_await ptr();
   co_await other();
   co_await (x);
   co_await(x);
   co_await (co_await nested());
   co_await *q;
   co_await run<Policy, 2>();
   co_await ::global_awaitable();
   int i = co_await a() + co_await b();
   int j = (co_await a()) + (co_await b());
   g(co_await h(), 1);
   co_yield 1;
   co_yield (1);
   co_yield(1);
   co_yield *q;
   co_yield make(1, 2);
   co_yield co_await get();
   for (int k = 0; k < 3; k++)
   {
      co_yield k;
   }
}

void plain()
{
   int a = 1;
   return;
}

T coro()
{
   int a = 1;
   co_return;
}

T lambdas(int n)
{
   return [](int x){ return x; }(1);
   co_return [](int x){ return x; }(1);
   co_yield [](int x){ return x; }(1);
   co_await [](int x) -> Task { co_return x; }(1);
   return [this](int x) mutable -> int { return x; };
   co_return [this](int x) mutable -> int { return x; };
   co_await [&, n](int x) -> Task { co_yield x; co_return n; }(2);
}
