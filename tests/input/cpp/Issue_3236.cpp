/* multi boolean expression without existing parentheses */
if (auto val = getValue(); val < 0 || val > 0)
{
}

/* multi boolean works */
if (val < 0 || val > 0)
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
