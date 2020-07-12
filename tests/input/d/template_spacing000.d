void fun() {
	if (something(a1, a2))
		return;
	if (something!a1)
		return;
	if (something!(a1, a2) )
		return;
	if (something!(a1, a2).Ptr)
		return;
	if (something!a1.Ptr)
		return;
}
