int SomeClass::method()
{
  switch (1) {
  case 0:
  {
    double v;
    break;
  }

  case 1:
  {
    double v;
    v = this->mat.operator()(0, 0);
    break;
  }

  case 2:
  {
    foo();
  }
  }
}
