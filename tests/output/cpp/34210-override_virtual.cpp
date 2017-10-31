struct A
{
	virtual void foo();
	virtual void bar() = 0;
	virtual void baz() const {
	}
};

struct B : public A
{
	virtual void foo() override;
	void bar() override {
	}
	void baz() const override {
	}
};
