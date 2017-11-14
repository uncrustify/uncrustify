void GetFoo(void)
                                                                     const
{
	return (m_Foo);
}

int GetFoo(void)
                                        throw (std::bad_alloc)
{
	return (m_Foo);
}

class foo {
	void bar(void)
	                                                                     const;
};

