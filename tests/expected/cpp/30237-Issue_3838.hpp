template<template < typename, typename ...> class IgnoredClass, typename Identify, typename ... IgnoredTypes >
         struct First<IgnoredClass<Identify, IgnoredTypes ...> > {
                          using type = Identify;
                      }
