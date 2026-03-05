// Test: align_func_proto_span_num_complex - Multi-level alignment with nested structures
// Tests various numbers of comment lines (1, 2, 3, 4) at each level

// Global level (level=0, brace_level=0)
VeryLongGlobalType global_func_one(int      param1,
                                   // Single comment
                                   double   param2,
                                   // Comment 1
                                   // Comment 2
                                   float    param3,
                                   // Comment 1
                                   // Comment 2
                                   // Comment 3
                                   long int param4,
                                   // Comment 1
                                   // Comment 2
                                   // Comment 3
                                   // Comment 4
                                   unsigned int param5
                                   );

namespace OuterNamespace {  // level=1, brace_level=0
LongNamespaceType ns_func_one(int      param1,
                              // Single comment
                              double   param2,
                              // Comment 1
                              // Comment 2
                              float    param3,
                              // Comment 1
                              // Comment 2
                              // Comment 3
                              long int param4,
                              // Comment 1
                              // Comment 2
                              // Comment 3
                              // Comment 4
                              unsigned int param5
                              );

class OuterClass {      // level=2, brace_level=1
public:
VeryLongMethodType outer_method_one(int      param1,
                                    // Single comment
                                    double   param2,
                                    // Comment 1
                                    // Comment 2
                                    float    param3,
                                    // Comment 1
                                    // Comment 2
                                    // Comment 3
                                    long int param4,
                                    // Comment 1
                                    // Comment 2
                                    // Comment 3
                                    // Comment 4
                                    unsigned int param5
                                    );

class InnerClass {          // level=3, brace_level=2
public:
LongInnerType inner_method_one(int      param1,
                               // Single comment
                               double   param2,
                               // Comment 1
                               // Comment 2
                               float    param3,
                               // Comment 1
                               // Comment 2
                               // Comment 3
                               long int param4,
                               // Comment 1
                               // Comment 2
                               // Comment 3
                               // Comment 4
                               unsigned int param5
                               );


struct DeepStruct {              // level=4, brace_level=3
    VeryLongStructType struct_method_one(int      param1,
                                         // Single comment
                                         double   param2,
                                         // Comment 1
                                         // Comment 2
                                         float    param3,
                                         // Comment 1
                                         // Comment 2
                                         // Comment 3
                                         long int param4,
                                         // Comment 1
                                         // Comment 2
                                         // Comment 3
                                         // Comment 4
                                         unsigned int param5
                                         );
};
};

// Back to level=2, brace_level=1
float outer_method_six (int      param1,
                        // Single comment
                        double   param2,
                        // Comment 1
                        // Comment 2
                        float    param3,
                        // Comment 1
                        // Comment 2
                        // Comment 3
                        long int param4,
                        // Comment 1
                        // Comment 2
                        // Comment 3
                        // Comment 4
                        unsigned int param5
                        );
};

// Back to level=1, brace_level=0
double ns_func_six(int      param1,
                   // Single comment
                   double   param2,
                   // Comment 1
                   // Comment 2
                   float    param3,
                   // Comment 1
                   // Comment 2
                   // Comment 3
                   long int param4,
                   // Comment 1
                   // Comment 2
                   // Comment 3
                   // Comment 4
                   unsigned int param5
                   );
}

// Back to global
long int global_func_six(int      param1,
                         // Single comment
                         double   param2,
                         // Comment 1
                         // Comment 2
                         float    param3,
                         // Comment 1
                         // Comment 2
                         // Comment 3
                         long int param4,
                         // Comment 1
                         // Comment 2
                         // Comment 3
                         // Comment 4
                         unsigned int param5
                         );
