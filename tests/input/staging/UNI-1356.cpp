// Hi,
// When using "space only" and "indent continue", I notice a wrong indentation in C language (at least)
// function call when the retrun value is assigned to a variable and the call is split in two or more line.
// In that case the indent is twice the indent set in "indent_continue"
// This only appears when "indent_with_tabs" is set to 0 "space only" and 1 
// "indent with tabs to brace level, align with spaces"
// Version tested:
// 0.59: good indentation
// 0.60: wrong indentation
// master (sha1 fc5228e): wrong indentation
// Here are some details about thats issue:
// orignal code
// The long line are manually split and not indented to test uncrustify indent

int main (int argc, char *argv[])
{
  double a_very_long_variable = test (foobar1, foobar2, foobar3, foobar4,
  foobar5, foobar6);

  double a_other_very_long = asdfasdfasdfasdfasdf + asdfasfafasdfa +
  asdfasdfasdf - asdfasdf + 56598;

  testadsfa (dfasdf, fdssaf, dsfasdf, sadfa, sadfas, fsadfa,
  aaafsdfa, afsd, asfdas, asdfa, asfasdfa, afsda, asfdasfds, asdfasf);

  return 0;
}
