template<class T>
class Foo<T>::Baz {
  Baz() noexcept
    : i(0)
  {}
};
