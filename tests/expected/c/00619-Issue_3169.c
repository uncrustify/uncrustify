#include <stdint.h>

void bar (void)
{

  int32_t       result;
  const int32_t other
    = 7;
}

#define P(bits)                   \
  void foo(void)                  \
  {                               \
                                  \
    int##bits##_t       result15; \
    const int##bits##_t other     \
      = 7;                        \
  }

#define Q(value)            \
  void baz(void)            \
  {                         \
                            \
    int32_t       result24; \
    const int32_t other     \
      = 7;                  \
  }

P(32)
Q(7)
