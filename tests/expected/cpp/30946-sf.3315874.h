#undef dot
#undef cross

extern "C" {
#include "data_types.h"
}
vec_       operator+( const vec_ &, const vec_ & );       /* v = v1 + v2   */
vec_       operator-( const vec_ &, const vec_ & );       /* v = v1 - v2    */
mat_       operator+( const mat_ &, const mat_ & );       /* m = m1 + m2    */
mat_       operator-( const mat_ &, const mat_ & );       /* m = m1 - m2    */
vec_       &operator+=( vec_ &, const vec_ & );           /* v += v2    */
mat_       &operator+=( mat_ &, const mat_ & );           /* m += m2    */
vec_       &operator-=( vec_ &, const vec_ & );           /* v -= v2    */
mat_       &operator-=( mat_ &, const mat_ & );           /* m -= m2    */
vec_       operator*( double, const vec_ & );             /* v = a * v1     */
mat_       operator*( double, const mat_ & );             /* m = a * m1     */
vec_       operator*( const vec_ &, double );             /* v = v1 * a    */
mat_       operator*( const mat_ &, double );             /* m = m1 * a    */
vec_       operator/( const vec_ &, double );             /* v = v1 / a    */
mat_       operator/( const mat_ &, double );             /* m = m1 / a  */
vec_       operator*=( vec_ &, const double a );          /* v *= a     */
vec_       operator/=( vec_ &, const double a );          /* v /= a       */
vec_       operator*( const mat_ &, const vec_ & );       /* v = m1 * v1  */
mat_       operator*( const mat_ &, const mat_ & );       /* m = m1 * m2  */
quat_      operator*( const quat_ &, const quat_ & );     /* q = q1 * q2  */
quat_      operator*( double, const quat_ & );            /* q = a * q1  */
quat_      operator*( const quat_ &, double );            /* q = q1 * a  */
quat_      operator/( const quat_ &, double );            /* q = q1 / a  */
vec_       operator-( const vec_ & );                     /* v = - v1    */
vec_       operator+( const vec_ & );                     /* v = + v1    */
mat_       operator-( const mat_ & );                     /* m = - m1    */
mat_       operator+( const mat_ & );                     /* m = + m1    */
quat_      operator+( const quat_ & );                    /* q = + q    */
quat_      operator-( const quat_ & );                    /* q = - q    */
quat_      &operator*=( quat_ &, const quat_ & );         /* q1 *= q2;  */
quat_      &operator+=( quat_ &, const quat_ & );         /* q1 += q2;  */
quat_      &operator*=( quat_ &, const double a );        /* q1 *= a;   */
quat_      operator+( const quat_ &q1, const quat_ &q2 ); /* q3  = q1 + q2  */
vec_       unit ( const vec_ & );                         /* unitize vec    */
quat_      unit ( const quat_ & );                        /* unitize quat   */
mat_       trans ( const mat_ & );                        /* transpose matrix  */
quat_      trans ( const quat_ & );                       /* transpose quat   */
double     dot ( const vec_, const vec_ );                /* vector dot product  */
vec_       cross ( const vec_, const vec_ );              /* vector cross product  */
