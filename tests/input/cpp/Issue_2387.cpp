namespace bar
{
void none();
};

void foo()
{
	namespace // does not
	x         // start a
	=         // namespace
	bar;

	x::none();
}
