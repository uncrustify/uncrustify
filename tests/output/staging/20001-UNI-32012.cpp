//Case:1

void foo()
{
    while (a < b && c > d)
    {
        i++;
    }
}

//Case2:

void foo()
{
    for (; a < b && c > d;)
    {
        i++;
    }
}

//Case3:

void foo()
{
    do
    {
        i++;
    }
    while (a < b && c > d);
}

//Case4
void foo()
{
    if (a < b && c > d)
    {
        i++;
    }
    else if (a < b && c > d)
    {
        i++;
    }
}
