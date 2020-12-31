void func() { }

// **ABC**
void func() { }
// *INDENT-ON*

void func() { }

/**
 * Function to solve for roots of a generic quartic polynomial of the following form:
 * \verbatim

   p(x) = a * x^4 + b * x^3 + c * x^2 + d * x + e,

   where a, b, c, d, and e are real coefficients

 * \endverbatim
 *
 * This object's tolerance defines a threshold for root solutions above which iterative methods will be employed to achieve the desired accuracy
 *
 * \verbatim - this should cause the following line to not wrap due to cmt_width
 * Upon success, the roots array contains the solution to the polynomial p(x) = 0
 * \endverbatim
 * + Return value on output:
 * - 0, if an error occurs (invalid coefficients)
 * - 1, if all roots are real
 * - 2, if two roots are real and two roots are complex conjugates
 * - 3, if the roots are two pairs of complex conjugates
 */
int solve(double a,
                    double b,
                 double c,
                 double d,
                     double e,
              std::complex<double> roots[4]);

/**
 * Function to solve for roots of a generic quartic polynomial of the following form:
 *

   p(x) = a * x^4 + b * x^3 + c * x^2 + d * x + e,
   where a, b, c, d, and e are real coefficients
 *
 * Upon success, root1, root2, root3, and root4 contain the solution to the polynomial p(x) = 0
 * + Return value on output:
 * - 0, if an error occurs (invalid coefficients)
 * - 1, if all roots are real
 * - 2, if two roots are real and two roots are complex conjugates
 * - 3, if the roots are two pairs of complex conjugates
 */
/* **ABC** */
              int solve(double a,
    double b,
                  double c,
            double d,
            double e,
                     std::complex<double> &root1,
                  std::complex<double> &root2,
            std::complex<double> &root3,
        std::complex<double> &root4);
/* ??DEF?? */
