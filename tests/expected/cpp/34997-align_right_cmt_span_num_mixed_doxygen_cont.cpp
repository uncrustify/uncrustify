/* Test file for align_right_cmt_doxygen_cont with span budget options. */

/* ============================================================
 * Section 1: continuations versus the comment budget
 * ============================================================ */

void test_cont_vs_cmt_budget(void)
{
    int a = 1;                  //!< comment a
    int bb_longer = 2;          //!< comment bb_longer...
    //!< ...continued once
    int ccc_even_longer = 3;    //!< comment ccc_even_longer...
    //!< ...continued once
    //!< ...continued twice
    int dddd_still_longer = 4;  //!< comment dddd_still_longer...
    //!< ...continued once
    //!< ...continued twice
    //!< ...continued three times
    int eeeee_the_last_one = 5; //!< comment eeeee_the_last_one
    int ffffff = 6;             //!< comment ffffff...
    //!< ...continued once
    // a standalone comment
    int ggggggg_the_last = 7;   //!< comment ggggggg_the_last
}

void test_standalone_still_costs_budget(void)
{
    int a = 1;               //!< comment a
    int bb_longer = 2;       //!< comment bb_longer
    // a standalone comment, not an 'after member' one
    int ccc_even_longer = 3; //!< comment ccc_even_longer
    // standalone C++ comment A
    // standalone C++ comment B
    int dddd_still_longer = 4; //!< comment dddd_still_longer
}

struct DetailedVersusCmtBudget
{
    /** Detailed a. */
    int a;               ///< Brief a.
    ///< continue a.
    /** Detailed bb_longer. */
    int bb_longer;       ///< Brief bb_longer.
    ///< continue bb_longer.
    /**
     * Detailed ccc_even_longer,
     * on several lines.
     */
    int ccc_even_longer; ///< Brief ccc_even_longer.
};

/* ============================================================
 * Section 2: continuations versus the empty line budget
 * ============================================================ */

void test_cont_vs_empty_budget(void)
{
    int a = 1;         //!< comment a...
    //!< ...continued once
    //!< ...continued twice

    int bb_longer = 2; //!< comment bb_longer...
    //!< ...continued once


    int ccc_even_longer = 3;    //!< comment ccc_even_longer
    int dddd = 4;               //!< comment dddd...
    //!< ...continued once

    int eeeee_the_last_one = 5; //!< comment eeeee_the_last_one
}

void test_empty_line_breaks_the_chain(void)
{
    int a = 1;               //!< comment a

    //!< a blank line separates it, so this is not a continuation
    int bb_longer = 2;       //!< comment bb_longer...
    //!< ...this one is a continuation
    int ccc_even_longer = 3; //!< comment ccc_even_longer
}

/* ============================================================
 * Section 3: continuations versus the preprocessor budget
 * ============================================================ */

void test_cont_vs_pp_budget(void)
{
    int a = 1;         //!< comment a...
    //!< ...continued once
    //!< ...continued twice
#define CONT_PP_1 1
    int bb_longer = 2; //!< comment bb_longer...
    //!< ...continued once
#define CONT_PP_2 2
#define CONT_PP_22 22
    int ccc_even_longer = 3;    //!< comment ccc_even_longer
    int dddd = 4;               //!< comment dddd...
    //!< ...continued once
#define CONT_PP_3 3
    int eeeee_the_last_one = 5; //!< comment eeeee_the_last_one
}

void test_pp_line_breaks_the_chain(void)
{
    int a = 1;               //!< comment a
#define CONT_PP_4 4
    //!< a #define separates it, so this is not a continuation
    int bb_longer = 2;       //!< comment bb_longer...
    //!< ...this one is a continuation
    int ccc_even_longer = 3; //!< comment ccc_even_longer
}

/* ============================================================
 * Section 4: everything mixed
 * ============================================================ */

void test_cont_mixed_lines(void)
{
    int a = 1;                       //!< comment a...
    //!< ...continued once

    /* standalone C comment 1 */
    int bb_longer = 2;               //!< comment bb_longer...
    //!< ...continued once
    //!< ...continued twice

#define CONT_MIX_1 1
    int ccc_even_longer = 3;         //!< comment ccc_even_longer...
    //!< ...continued once

#define CONT_MIX_2 2
    // standalone C++ comment A
    int dddd_a_bit_longer = 4;       //!< comment dddd_a_bit_longer...
    //!< ...continued once
    //!< ...continued twice
    //!< ...continued three times
    /* standalone C comment X */
#define CONT_MIX_3 3

    int eeeee_even_longer_still = 5; //!< comment eeeee_even_longer_still...
    //!< ...continued once


    int ffffff_the_very_last = 6; //!< comment ffffff_the_very_last
}

/* ============================================================
 * Section 5: markers and block comments
 * ============================================================ */

/* The two C++ 'after member' markers continue each other. */
void test_cont_marker_mixing(void)
{
    int a = 1;               //!< comment a...
    ///< ...continued with the other marker
    int bb_longer = 2;       ///< comment bb_longer...
    //!< ...continued with the other marker
    int ccc_even_longer = 3; //!< comment ccc_even_longer
}

/* A block comment on several lines and one on a single line continue each
 * other in both directions. */
void test_cont_after_multiline_block(void)
{
    int a = 1;               /*!< comment a
                                  written on two lines */
    /*!< the continuation of a multi-line block comment */
    int bb_longer = 2;       /*!< comment bb_longer */
    /*!< a continuation
         itself written on two lines */
    int ccc_even_longer = 3; /*!< comment ccc_even_longer */
}

/* ============================================================
 * Section 6: class attributes crossing every kind of line
 * ============================================================ */

class DocumentedAttributes
{
private:
    int m_n {1};                                      //!< the counter
    double m_relative_tolerance {1e-12};              //!< tolerance used when...
    //!< ...comparing two coordinates

#define ATTR_PP_1 1
    bool m_ok {false};                                //!< whether the last call succeeded

    /* a standalone C comment between two attributes */
    unsigned long m_number_of_processed_elements {0}; //!< how many elements...
    //!< ...have been processed so far...
    //!< ...reset by every call to clear()
    char m_c {'x'};                                   //!< a single character


    short m_small_one {0}; //!< a short one...
    //!< ...documented on two lines
};
