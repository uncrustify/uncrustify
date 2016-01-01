int & aa(int & x,int & b);   // Sp Before Byref Func, Sp After Byref Func, Sp Before Byref, Sp After Byref
int aa(int & x,int &)     // Sp Before Byref, Sp Before Unnamed Byref, Sp After Byref
{
	b = aa(x,b);
	c = aa(& y,& d);     // Sp Addr
}
