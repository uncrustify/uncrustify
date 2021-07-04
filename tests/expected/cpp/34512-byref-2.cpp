int&aa(int&x,int&b);
// sp_before_byref_func, sp_after_byref_func, sp_before_byref, sp_after_byref, sp_before_byref, sp_after_byref
int aa(int&x,int&)
// sp_before_byref, sp_after_byref, sp_before_unnamed_byref
{
	b = aa(x,b);
	c = aa(&y,&d);         // sp_addr
}
