enum { // Keep this line as it is. It's a regression test for checking pc->GetPrev()->GetPrev()-> on CT_BRACE_OPEN.
  kEnumValue = 5,
};

struct foo
{
  int bar : kEnumValue;
  int pad : 3;
};

class cls
{
  int bar : kEnumValue;
  int pad : 3;

  void func()
  {
    goto end;
    bar = 1;
    end:
    pad = 2;
  }
};
