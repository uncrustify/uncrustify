/* multi boolean */
if ((va1 < 0) || (va2 > 0))
{
}


/* single condition */
if (auto val = getValue(); condition(val))
{
}

/* multi boolean expression with existing parentheses */
if (auto val = getValue(); (val < 0) || (val > 0))
{
}

/* multi boolean expression without existing parentheses */
if (auto val = getValue(); (val < 0) || (val > 0))
{
}
