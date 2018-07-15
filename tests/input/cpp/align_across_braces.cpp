enum foo // comment
{
  long_enum_value, // these comments should be aligned
  another_value, // with each other, but not
  shorter, // with the first line
}; // this comment should start a new group
void bar(); // this one should align with the previous line
