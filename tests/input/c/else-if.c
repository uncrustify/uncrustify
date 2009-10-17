int test (int A, int B) {

  int C;

  if (A == 0)
    if (B == 0)
      C = 1;
    else if (B == 1)
      C = 2;
    else
      C = 3;
  else if (A == 1)
    if (B == 0)
      C = 4;
    else if (B == 1)
      C = 5;
    else
      C = 6;
  else
    if (B == 0)
      C = 7;
    else if (B == 1)
      C = 8;
    else
      C = 9;

  return C;
}

