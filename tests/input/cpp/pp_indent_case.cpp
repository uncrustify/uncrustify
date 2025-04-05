// Example for case in a preprocessor statement
// Config uses more than tested option, uses:
// pp_if_indent_code  = true	  to enable preprocessor indent
// pp_indent_case	  = false	  to override preprocessor indent for case blocks
switch(...)
{
case 1:
case 2:
{
	int v;
	...
}
break;

#if (USE_FIVE)
	case 3:
		doFive();
		break;
#endif

default:
	break;
}