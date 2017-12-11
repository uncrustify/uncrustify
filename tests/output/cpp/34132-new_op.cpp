Foo* foo = new Foo(a,v);

Foo* foo = new ( ptr,std::nothrow ) Foo[];
Foo* foo = new ( ptr ) Foo();
Foo* foo = new ( FOO(ptr)) Foo();

Foo* foo = new  (  ptr,std::nothrow  )  Foo[];
Foo* foo = new  (  ptr  )  Foo();
Foo* foo = new  (  FOO(ptr))  Foo();
