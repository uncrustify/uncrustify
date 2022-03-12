// Example for preprocessor statement in a function definition
// Config uses more than tested option, uses:
// pp_if_indent_code  = 1	  to enable preprocesser indent
// pp_indent_brace	  = 0	  to override preprocessor indent for braces
MyClass::MyClass()
{
	if(isSomething)
	{
		DoSomething();
	}

#if (USE_FIVE)
	{
		DoSomethingAlso();
	}
#endif

#if (USE_SIX)
	{
		Six mySix;
		DoSomethingWithSix(mySix);
	}
#endif
}
