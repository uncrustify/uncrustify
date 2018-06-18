namespace {

  /* If we're in the middle of the original line, copy the string
   * only up to the cursor position into buf, so tab completion
   * will result in buf's containing only the tab-completed
   * path/filename. */

  class Test {

    Test() {}
    ~Test() {}

    /** Call this method to
     *      run the test
     *
     *  \param n test number
     *  \returns the test result
     */
    bool Run(int n);

    /** Call this method to
     *  stop the test
     *
     * \param n test number
     * \returns the test result
     */
    bool Run(int n);

  };

}

