void foo()
{
	obj.cb += () => { };
	func();
	obj.cb += (p0) => { };
	func();
	obj.cb += (p0, p1) => { };
	func();
	Action a = delegate { };
	func();
}
