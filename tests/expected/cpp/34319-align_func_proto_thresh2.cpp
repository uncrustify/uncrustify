class AlignFuncProtoTest {
public:
virtual void                                                                             test1(std::wstring & name, std::pair<Space1::Space2::SomeType, Space1::Space2::otherType> param1) = 0;
virtual SomeLongType                                                                     findSomeLongType()                                                                                = 0;
virtual Some::Type                                                                       test2()                                                                                           = 0;
virtual SomeNameSpace::TypeA                                                             test3()                                                                                           = 0;
virtual SomeNameSpace::SubNameSpace1::TypeA                                              test4()                                                                                           = 0;
virtual SomeNameSpace::SubNameSpace1::SubNameSpace2::TypeB                               test5()                                                                                           = 0;
virtual SomeNameSpace::SubNameSpace1::SubNameSpace2::SubNameSpace3::TypeC                test6()                                                                                           = 0;
virtual SomeNameSpace::SubNameSpace1::SubNameSpace2::SubNameSpace3::SubNameSpace4::TypeD test7()                                                                                           = 0;
double test5();
void   test6();
SomeLongNamespace::OtherLongNamespace::SomeLongType findSomeLongType();
void   test7();
void   test8();
void   test9();
SomeLongNamespace::SomeLongType long_var;
SomeNameSpace::SubNameSpace1::SubNameSpace2::SubNameSpace3::SubNameSpace4::SubNameSpace5::TypeE test7();
}
