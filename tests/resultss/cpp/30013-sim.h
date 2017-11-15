namespace ns {
template<typename T, template<typename>class TOtherClass>
class Example
{
  int foo;
}

}

template<class T>
class Example
{
  T getValue() const;

  /** A pointer to a T returning function in the software environment */
  T (FunctionProvider::* pF)();

};


#if !defined(EVERYTHING_OK)
#error Define EVERYTHING_OK if you would like to compile your code \
  or not if you would like to stop!
#endif


template<class V>
class Example
{
  Vector2<V>() :
    x(1),
    y(1)
  {}

  Vector2<double>() :
    x(1.0),
    y(1.0)
  {}

  Vector2<float>() :
    x(1.0f),
    y(1.0f)
  {}

  V x;
  V y;

};
