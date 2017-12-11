/* A collection of all the different known operator prototypes in C++ */

// arithmetic operators
Type1 operator +(const Type1& a);                          // +a
Type1 operator +(const Type1& a, const Type2& b);          // a + b
Type1& operator++(Type1& a);                               // ++a
Type1 operator ++(Type1& a, int);                          // a++
Type1& operator+=(Type1& a, const Type1& b);               // a += b
Type1 operator -(const Type1& a);                          // -a
Type1& operator--(Type1& a);                               // --a
Type1 operator --(Type1& a, int);                          // a--
Type1& operator-=(Type1& a, const Type1& b);               // a -= b
Type1 operator *(const Type1& a, const Type1& b);          // a * b
Type1& operator*=(Type1& a, const Type1& b);               // a *= b
Type1 operator /(const Type1& a, const Type1& b);          // a / b
Type1& operator/=(Type1& a, const Type1& b);               // a /= b
Type1 operator %(const Type1& a, const Type1& b);          // a % b
Type1& operator%=(Type1& a, const Type1& b);               // a %= b

// comparison operators
bool operator<(const Type1& a, const Type1& b);            // a < b
bool operator<=(const Type1& a, const Type1& b);           // a <= b
bool operator>(const Type1& a, const Type1& b);            // a > b
bool operator>=(const Type1& a, const Type1& b);           // a >= b
bool operator!=(const Type1& a, const Type1& b);           // a != b
bool operator==(const Type1& a, const Type1& b);           // a == b

// logical operators
bool operator!(const Type1& a);                            // !a
bool operator&&(const Type1& a, const Type1& b);           // a && b
bool operator||(const Type1& a, const Type1& b);           // a || b

// bitwise operators
Type1 operator <<(const Type1& a, const Type1& b);         // a << b
Type1& operator<<=(Type1& a, const Type1& b);              // a <<= b
Type1 operator >>(const Type1& a, const Type1& b);         // a >> b
Type1& operator>>=(Type1& a, const Type1& b);              // a >>= b
Type1 operator ~(const Type1& a);                          // ~a
Type1 operator &(const Type1& a, const Type1& b);          // a & b
Type1& operator&=(Type1& a, const Type1& b);               // a &= b
Type1 operator |(const Type1& a, const Type1& b);          // a | b
Type1& operator|=(Type1& a, const Type1& b);               // a |= b
Type1 operator ^(const Type1& a, const Type1& b);          // a ^ b
Type1& operator^=(Type1& a, const Type1& b);               // a ^= b

// other operators
Type1& Type1::operator=(const Type1& b);                   // a = b
void operator         ()(Type1& a);                        // a()
const Type2& operator [](const Type1& a, const Type1& b);  // a[b]
Type2& operator       *(const Type1& a);                   // *a
Type2* operator       &(const Type1& a);                   // &a
Type2* Type1::operator->();                                // a->b
Type1::operator       type();                              // (type)a
Type2& operator       ,(const Type1& a, Type2& b);         // a, b
void *Type1::operator new(size_t x);                       // new Type1
void *Type1::operator new[](size_t x);                     // new Type1[n]
void *Type1::operator delete(size_t x);                    // delete a
void *Type1::operator delete[](size_t x);                  // delete [] a

// Misc examples
int& operator *();
Foo::operator const char *();
Foo::operator const Bar&();

