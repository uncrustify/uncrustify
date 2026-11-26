// Test: a trailing comment of a line moving right when it is re-indented,
// with sp_before_tr_cmt

void g(int a)
{
int x   = 1; // c1
int w   = 1;   // three spaces in the input
int g   = 2;// no space in the input
if (a) a   = 2; // after a statement without braces
}
