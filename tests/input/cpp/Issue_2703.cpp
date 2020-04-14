#define DEFINE_OPERATORS(classT, flagsT)          \
    inline classT::flagsT                         \
    operator&(const classT::flagsT&          lh1, \
              const classT::flagsT::EnumType rh1) \
    {                                             \
        return classT::flagsT(lhs) &= rhs;        \
    }                                             \
                                                  \
    inline classT::flagsT                         \
    operator&(const classT::flagsT::EnumType lh2, \
              const classT::flagsT&          rh2) \
    {                                             \
        return classT::flagsT(lhs) &= rhs;        \
    }
