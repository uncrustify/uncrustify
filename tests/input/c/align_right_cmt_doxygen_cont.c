/* Test file for align_right_cmt_doxygen_cont. */

/* ============================================================
 * Section 1: local variables
 * ============================================================ */

void test_c_cmt_doxygen_cont_locals(void)
{
    int a = 1; /*!< comment a */
    int bb = 2; /*!< comment bb... */
    /*!< ...continued */
    int ccc = 3; /*!< comment ccc... */
/*!< ...continued in column 1 */
                          /*!< ...continued further right */
    int dddd = 4; /*!< comment dddd... */
      /*!< ...continued */
         /*!< ...continued */
    /*!< ...continued */
    int eeeee_long = 5; /*!< comment eeeee_long */
}

/* ============================================================
 * Section 2: continuations already aligned
 * ============================================================ */

void test_c_cmt_doxygen_cont_aligned(void)
{
    int a = 1;               /**< comment a */
    int bb = 2;              /**< comment bb... */
                             /**< ...continued */
    int ccc = 3;             /**< comment ccc... */
/**< ...continued in column 1 */
    int eeeee_long = 5;      /**< comment eeeee_long */
}

/* ============================================================
 * Section 3: struct members, every 'after member' marker
 * ============================================================ */

struct attributes
{
    int m_a; /*!< the a */
    long m_bbbb; /*!< the bbbb... */
/*!< ...continued */
    int m_cc; /**< the cc... */
                /**< ...continued */
    unsigned m_ddddd; //!< the ddddd...
        //!< ...continued
    double m_eeeeeeee; ///< the eeeeeeee...
                                       ///< ...continued
};

/* ============================================================
 * Section 4: globals and enum
 * ============================================================ */

int g_a = 1; /*!< global a */
static double g_tolerance = 1e-12; /*!< global tolerance... */
    /*!< ...continued */
int g_ccc = 3; /*!< global ccc */

enum color
{
    RED, /*!< red... */
/*!< ...continued */
    GREEN_LONGER, /*!< green */
};

/* ============================================================
 * Section 5: one continuation, in every original column
 * ============================================================ */

struct columns
{
    int bb; /*!< bb: at the code indent */
    /*!< ...continued */
    int ccc; /*!< ccc: in column 1 */
/*!< ...continued */
    int dddd; /*!< dddd: two columns to the left */
            /*!< ...continued */
    int eeeee; /*!< eeeee: far to the right */
                                      /*!< ...continued */
};

/* ============================================================
 * Section 6: continuations spread over several columns
 * ============================================================ */

struct chains
{
    int a; /*!< a... */
/*!< ...in column 1 */
    /*!< ...at the code indent */
                          /*!< ...far to the right */
    long bb; /**< bb... */
               /**< ...somewhere */
    /**< ...at the code indent */
};

/* ============================================================
 * Section 7: heads which are not 'after member' comments
 * ============================================================ */

struct heads
{
    int a; /* a plain trailing comment... */
    /*!< ...continued by an 'after member' one */
    int bb; /*!< a C comment spread
                 over two lines... */
/*!< ...continued in column 1 */
    int ccc; /*!< ccc */
};

/* ============================================================
 * Section 8: a head after a closing brace, and nested levels
 * ============================================================ */

struct outer
{
    struct inner
    {
        int a; /*!< a... */
        /*!< ...continued */
    } inner; /*!< inner... */
/*!< ...continued */
    int bb; /*!< bb */
};

/* ============================================================
 * Section 9: detailed description before, brief and
 *            continuation after
 * ============================================================ */

struct point
{
    /** Detailed x. */
    int x; ///< Brief x.
 ///< continue x.
    /** Detailed y. */
    int yy; /**< Brief yy. */
        /**< continue yy. */
    /**
     * Detailed zzz,
     * on several lines.
     */
    double zzz; ///< Brief zzz.
                    ///< continue zzz.
};

/* ============================================================
 * Section 10: what must NOT be taken for a continuation
 * ============================================================ */

void test_c_cmt_doxygen_cont_no_match(void)
{
    int a = 1; /* a plain trailing comment */
    /* a standalone comment describing the next statement */
    int bb = 2; /* a plain trailing comment */

    int ccc = 3; /*!< comment ccc */

    /*!< separated from the comment above by a blank line */
    int dddd = 4; /*!< comment dddd */
    /*! not an 'after member' marker */
    int eeeee = 5; /*!< comment eeeee */
    /* not an 'after member' marker either */
    int ffffff = 6; /*!< comment ffffff */

    /* a standalone comment */
    /*!< below a standalone comment */
    int g = 7; /*!< C comment */
               //!< C++ comment below a C one
    int hh = 8; //!< C++ comment
                /*!< C comment below a C++ one */
    int iii = 9; /*!< C comment */
    /*!< followed by code */ int jjjj = 10;
}

#if 1
#endif /*!< after #endif */
/*!< below a comment after #endif */
