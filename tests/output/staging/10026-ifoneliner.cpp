void foo()
{
    if (a) a++;
    if (!a) b++ else if (!b) a++ else c++;

    if (b)
    { b++ }
    else if (c)
    { c++ }
    else { d++ }
}
