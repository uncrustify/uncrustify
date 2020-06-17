struct A
{
	static if (true)
		void f() {
		}
}
struct B
{
	static if(true) {
		int a;
	}
	else{
		int e;
	}
}