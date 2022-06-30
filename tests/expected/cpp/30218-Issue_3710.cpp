void foo()
{
	PyErr_SetString( PyExc_KeyError, sstr.str().c_str() );
}
