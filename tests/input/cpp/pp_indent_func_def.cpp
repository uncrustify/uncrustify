// Example of function definitions inside of preprocessor statements
// Config uses more than tested option, uses:
// pp_if_indent_code  = true	  to enable preprocesser indent
// pp_indent_func_def = false	  to override preprocessor indent for function definitions
int x = 1;
#if (USE_AWESOME_FUNCTIONS)
	void MyClass::SomeAwesomeFunction()
	{
		DoSomethingInAFunction();
	}
#endif