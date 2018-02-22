void Foo()
{
	void* pAllocatedInput = new (std::nothrow) PointerTouchInfo[inputCount];
	void* int = new (std::nothrow) int[10];
}