/* Test: align_assign_span_num_cmt_lines - Comment lines between assignments */

void test_function()
{
    VeryLongVariableType var1 = 100;
    /* Single comment */
    int var2                  = 200;
    /* Comment 1 */
    /* Comment 2 */
    double var3 = 300.0;
    /* Comment 1 */
    /* Comment 2 */
    /* Comment 3 */
    unsigned char var4 = 400;

    long int var5 = 500;
}
