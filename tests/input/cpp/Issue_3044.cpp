int main()
{
  int af;
  int A;
  int B;
  switch (af)
    {
    case 1:
      B = 2;
    case 2:
      return 1;
    case 3:
      A = 1;
      break;
#ifdef ALL_THE_CASE
    case 4:
      return 2;
#endif
#ifdef ALL_THE_CASE
    case 5:
      B = 2;
#endif
    case (6):
      B=13;
#ifdef PART_OF_THE_CASE_UNDER
      A=1;
#endif
      break;
    case (7) :
#ifdef PART_OF_THE_CASE_ABOVE
      A=5;
#endif
      B=7;
      break;
    default:
      B= 50;
    }
}
