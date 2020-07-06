/* Short
 * Very long statement, needs much wrapping */

// Make sure C++ comments are properly wrapped,
// we do not want anything
// strange
class Test : public Beta
{
    /* Short
     * Very long statement, needs wrapping */
    void func()
    {
        /** Make sure
         * not to break anything, even in longer cases. */

        // Make sure C++ comments are properly wrapped,
        // we do not want anything
        // strange
    }

    /**
     * The quick brown fox jumps over
     * the lazy dog
     */
};
