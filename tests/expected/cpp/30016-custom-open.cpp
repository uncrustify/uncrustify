
void className::set(const objectName& obj)
{
	statement1();
	MACRO_BEGIN_STUFF(param)
		DOSTUFF(params)
	MACRO_ELSE_STUFF()
		DOMORESTUFF(moreparams)
		junk = 1;
		MACRO2_BEGIN_STUFF
			junk += 3;
		MACRO2_ELSE_STUFF
			junk += 4;
		MACRO2_END_STUFF
		DOLASTSTUFF(lastparams)
	MACRO_END_STUFF()
	statement2();
}


MACRO2_BEGIN_STUFF
	// comment
MACRO2_ELSE_STUFF
	/* Comment */
MACRO2_END_STUFF

