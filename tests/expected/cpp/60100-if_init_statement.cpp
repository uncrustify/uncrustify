/* multi boolean works, but removes ' ' before '||' */
if ((val < 0) || (val > 0))
{
}

/* single condition works */
if (auto val = getValue(); condition(val))
{
}

/* multi boolean expression with existing parentheses will not change */
if (auto val = getValue(); (val < 0) || (val > 0))
{
}

/* multi boolean expression without existing parentheses produces invalid C++ code */
if (auto val = getValue(); (val < 0) || (val > 0))
{
}
