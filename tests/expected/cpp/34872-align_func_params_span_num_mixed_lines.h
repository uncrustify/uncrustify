// Test: align_func_proto_span_num_mixed - Multi-level alignment with nested structures and mixed empty, pp and cmt lines.

// Global level (level=0, brace_level=0)
VeryLongGlobalType global_func_one(int          param1,
				   #ifdef A_FEATURE
                                   double       param2,
				   #endif
                                   // A comment
                                   float        param3,
				   #ifdef B_FEATURE

                                   long int     param4,
				   #endif
                                   // A comment

                                   unsigned int param5
                                   );

namespace OuterNamespace {  // level=1, brace_level=0
LongNamespaceType ns_func_one(int    param1,
				  #ifdef A_FEATURE
                              double param2,
				  #endif
				  #ifdef B_FEATURE
                              // A Comment
                              float  param3 = 5.6,     //!< A comment on the same line
				  #endif // end of #ifdef
				  #ifdef B_FEATURE
                              // Another comment


                              long int param4,
				  #endif
				  #ifdef C_FEATURE

				  #endif
                              unsigned int param5
                              );

template <class T>
class OuterClass {      // level=2, brace_level=1
public:
VeryLongMethodType outer_method_one(T&              param1,
					    #ifdef A_FEATURE
                                    const T&&       param2,
					    #endif
					    #ifdef B_FEATURE
                                    // A Comment
                                    std::vector<T>& param3,
					    #endif
					    #ifdef B_FEATURE
                                    // A comment for this
                                    // because it is strange...
                                    long int        param4,
					    #endif
					    #ifdef C_FEATURE
					    #endif
                                    OuterNamespace::LongNamespaceType& param5
                                    );

};

}

VeryLongGlobalType another_global_func(int param1,


                                       unsigned int param2
                                       );

VeryLongGlobalType third_global_func(int param1,
#if 1
#if 1
#if 1
                                     unsigned int param2
#endif
#endif
#endif
                                     );

VeryLongGlobalType fourth_global_func(int param1,
                                      // cmt 1
                                      // cmt 2
                                      // cmt 3
                                      // cmt 4
                                      unsigned int param2
                                      );
