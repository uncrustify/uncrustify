class foo
{
friend void bar();
friend void none();
template <typename T> friend vector<T> vec();
};
