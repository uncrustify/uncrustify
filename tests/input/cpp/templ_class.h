template<typename T, template<typename> class SpecialClass>
class Example
{
  // Copy constructor with other variants of Example
  template<template<typename> class OtherSpecialClass>
  Example(const Example<T, OtherSpecialClass>& other)
  {
    // do something useful here
  }

  /** The normal member var based on the template arguments */
  SpecialClass<T> memberVar;

};


