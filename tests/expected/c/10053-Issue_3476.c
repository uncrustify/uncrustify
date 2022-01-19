int a;
int b;
int c;

int main()
{
    if (a
      & b)
    {
        return 1;
    }

    if ( a
       | b )
    {
        return 2;
    }

    if (  a
        ^ b)
    {
        return 3;
    }

    c = a
     + b;

    c = a
     - b;

    c =  a
       * b;

    c =   a
        / b;

    return 0;
}
