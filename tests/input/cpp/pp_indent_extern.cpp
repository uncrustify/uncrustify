// Example for extern "C" blocks inside preprocessor statements
// Config uses more than tested option, uses:
// pp_if_indent_code  = true	  to enable preprocesser indent
// pp_indent_extern	  = false	  to override preprocessor indent for braces
int x = 1;
#ifdef __cplusplus
	extern "C" {

	void some_c_function
	(
		void
	);
    }
#endif